/*
 * Copyright (c) Kudo Chien.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include "V8Runtime.h"

#include <glog/logging.h>
#include <mutex>
#include <sstream>
#include "HostProxy.h"
#include "JSIV8ValueConverter.h"
#include "V8Inspector.h"
#include "V8PointerValue.h"
#include "jsi/jsilib.h"

namespace jsi = facebook::jsi;

namespace rnv8 {

namespace {

const char kHostFunctionProxyProp[] = "__hostFunctionProxy";

} // namespace

// static
std::unique_ptr<v8::Platform> V8Runtime::s_platform = nullptr;
std::mutex s_platform_mutex; // protects s_platform

V8Runtime::V8Runtime(std::unique_ptr<V8RuntimeConfig> config, 
std::shared_ptr<facebook::react::MessageQueueThread> jsQueue)
    : config_(std::move(config)) {
  {
    const std::lock_guard<std::mutex> lock(s_platform_mutex);
    if (!s_platform) {
      s_platform = v8::platform::NewDefaultPlatform();
      v8::V8::InitializeICU();
      v8::V8::InitializePlatform(s_platform.get());
      v8::V8::SetFlagsFromString("--nolazy");
      v8::V8::Initialize();
    }
  }

  arrayBufferAllocator_.reset(
      v8::ArrayBuffer::Allocator::NewDefaultAllocator());
  v8::Isolate::CreateParams createParams;
  createParams.array_buffer_allocator = arrayBufferAllocator_.get();
  if (config_->snapshotBlob) {
    snapshotBlob_ = std::make_unique<v8::StartupData>();
    snapshotBlob_->data = config_->snapshotBlob->c_str();
    snapshotBlob_->raw_size = static_cast<int>(config_->snapshotBlob->size());
    createParams.snapshot_blob = snapshotBlob_.get();
  }

  isolate_ = v8::Isolate::New(createParams);
#if defined(__ANDROID__)
  if (!config_->timezoneId.empty()) {
    isolate_->DateTimeConfigurationChangeNotification(
        v8::Isolate::TimeZoneDetection::kCustom, config_->timezoneId.c_str());
  }
#endif
  v8::Locker locker(isolate_);
  v8::Isolate::Scope scopedIsolate(isolate_);
  v8::HandleScope scopedHandle(isolate_);
  context_.Reset(isolate_, CreateGlobalContext(isolate_));
  v8::Context::Scope scopedContext(context_.Get(isolate_));
  jsQueue_ = jsQueue;
  if (config_->enableInspector) {
    inspectorClient_ = std::make_shared<InspectorClient>(
        jsQueue_, context_.Get(isolate_), config_->appName, config_->deviceName);
    inspectorClient_->ConnectToReactFrontend();
  }
}

V8Runtime::V8Runtime(
    const V8Runtime *v8Runtime,
    std::unique_ptr<V8RuntimeConfig> config)
    : config_(std::move(config)) {
  isSharedRuntime_ = true;
  // We don't need to register another idle taskrunner again
  isRegisteredIdleTaskRunner_ = true;
  isolate_ = v8Runtime->isolate_;
  jsQueue_ = v8Runtime->jsQueue_;

  v8::Locker locker(isolate_);
  v8::Isolate::Scope scopedIsolate(isolate_);
  v8::HandleScope scopedHandle(isolate_);
  context_.Reset(isolate_, CreateGlobalContext(isolate_));

  auto localContext = context_.Get(isolate_);
  localContext->SetSecurityToken(
      v8Runtime->context_.Get(isolate_)->GetSecurityToken());

  bool inheritProtoResult =
      localContext->Global()
          ->GetPrototype()
          .As<v8::Object>()
          ->SetPrototype(
              localContext,
              v8Runtime->context_.Get(isolate_)->Global()->GetPrototype())
          .FromJust();
  if (!inheritProtoResult) {
    LOG(ERROR) << "Unable to inherit prototype from parent shared runtime.";
  }

  if (config_->enableInspector) {
    inspectorClient_ = std::make_shared<InspectorClient>(
        jsQueue_, context_.Get(isolate_), config_->appName, config_->deviceName);
    inspectorClient_->ConnectToReactFrontend();
  }
}

V8Runtime::~V8Runtime() {
  {
    v8::Locker locker(isolate_);
    v8::Isolate::Scope scopedIsolate(isolate_);
    v8::HandleScope scopedHandle(isolate_);
    if (inspectorClient_) {
      inspectorClient_.reset();
    }

    context_.Reset();
  }
  if (!isSharedRuntime_) {
    isolate_->Dispose();
  }
  // v8::V8::Dispose();
  // v8::V8::DisposePlatform();
}

v8::Local<v8::Context> V8Runtime::CreateGlobalContext(v8::Isolate *isolate) {
  v8::HandleScope scopedHandle(isolate);
  v8::Local<v8::ObjectTemplate> global = v8::ObjectTemplate::New(isolate_);
  global->Set(
      v8::String::NewFromUtf8(isolate, "_v8runtime", v8::NewStringType::kNormal)
          .ToLocalChecked(),
      v8::FunctionTemplate::New(isolate, V8Runtime::GetRuntimeInfo));
  return v8::Context::New(isolate_, nullptr, global);
}

jsi::Value V8Runtime::ExecuteScript(
    v8::Isolate *isolate,
    const v8::Local<v8::String> &script,
    const std::string &sourceURL) {
  v8::HandleScope scopedHandle(isolate);
  v8::TryCatch tryCatch(isolate);

  v8::MaybeLocal<v8::String> sourceURLValue = v8::String::NewFromUtf8(
      isolate,
      sourceURL.c_str(),
      v8::NewStringType::kNormal,
      static_cast<int>(sourceURL.length()));
  v8::ScriptOrigin origin(isolate, sourceURLValue.ToLocalChecked());

  v8::Local<v8::Context> context(isolate->GetCurrentContext());

  auto codecache = LoadCodeCacheIfNeeded(config_->codecachePath);
  v8::ScriptCompiler::CachedData *cachedData = codecache.release();

  std::unique_ptr<v8::ScriptCompiler::Source> source =
      UseFakeSourceIfNeeded(origin, cachedData);
  if (!source) {
    source = std::make_unique<v8::ScriptCompiler::Source>(
        script, origin, cachedData);
  }

  v8::Local<v8::Script> compiledScript;
  if (!v8::ScriptCompiler::Compile(
           context,
           source.release(),
           cachedData ? v8::ScriptCompiler::kConsumeCodeCache
                      : v8::ScriptCompiler::kNoCompileOptions)
           .ToLocal(&compiledScript)) {
    ReportException(isolate, &tryCatch);
    return {};
  }

  if (cachedData && cachedData->rejected) {
    LOG(INFO) << "[rnv8] cache missed.";
  }
  SaveCodeCacheIfNeeded(compiledScript, config_->codecachePath, cachedData);

  v8::Local<v8::Value> result;
  if (!compiledScript->Run(context).ToLocal(&result)) {
    assert(tryCatch.HasCaught());
    ReportException(isolate, &tryCatch);
    return {};
  }

  RegisterIdleTaskRunnerIfNeeded();

  return JSIV8ValueConverter::ToJSIValue(isolate, result);
}

void V8Runtime::RegisterIdleTaskRunnerIfNeeded() {
  if (isRegisteredIdleTaskRunner_) {
    return;
  }

  global().setProperty(
      *this,
      "__v8OnIdleCallback",
      jsi::Function::createFromHostFunction(
          *this,
          jsi::PropNameID::forAscii(*this, "__v8OnIdleCallback"),
          0,
          [](jsi::Runtime &runtime,
             jsi::Value const &thisValue,
             const jsi::Value *args,
             size_t count) {
            V8Runtime *v8Runtime = dynamic_cast<V8Runtime *>(&runtime);
            v8Runtime->OnIdle();
            return jsi::Value();
          }));
  global()
      .getPropertyAsFunction(*this, "requestIdleCallback")
      .call(*this, global().getPropertyAsFunction(*this, "__v8OnIdleCallback"));

  isRegisteredIdleTaskRunner_ = true;
}

void V8Runtime::OnIdle() {
  v8::Locker locker(isolate_);
  v8::Isolate::Scope scopedIsolate(isolate_);
  v8::HandleScope scopedHandle(isolate_);
  v8::Context::Scope scopedContext(context_.Get(isolate_));

  while (v8::platform::PumpMessageLoop(
      s_platform.get(),
      isolate_,
      v8::platform::MessageLoopBehavior::kDoNotWait)) {
    continue;
  }
  global()
      .getPropertyAsFunction(*this, "requestIdleCallback")
      .call(*this, global().getPropertyAsFunction(*this, "__v8OnIdleCallback"));
}

void V8Runtime::ReportException(v8::Isolate *isolate, v8::TryCatch *tryCatch)
    const {
  v8::HandleScope scopedHandle(isolate);
  std::string exception =
      JSIV8ValueConverter::ToSTLString(isolate, tryCatch->Exception());
  v8::Local<v8::Message> message = tryCatch->Message();
  if (message.IsEmpty()) {
    // V8 didn't provide any extra information about this error; just
    // print the exception.
    throw jsi::JSError(const_cast<V8Runtime &>(*this), exception);
    return;
  } else {
    std::ostringstream ss;
    v8::Local<v8::Context> context(isolate->GetCurrentContext());

    // Print (filename):(line number): (message).
    ss << JSIV8ValueConverter::ToSTLString(
              isolate, message->GetScriptOrigin().ResourceName())
       << ":" << message->GetLineNumber(context).FromJust() << ": " << exception
       << std::endl;

    // Print line of source code.
    ss << JSIV8ValueConverter::ToSTLString(
              isolate, message->GetSourceLine(context).ToLocalChecked())
       << std::endl;

    // Print wavy underline (GetUnderline is deprecated).
    int start = message->GetStartColumn(context).FromJust();
    for (int i = 0; i < start; i++) {
      ss << " ";
    }
    int end = message->GetEndColumn(context).FromJust();
    for (int i = start; i < end; i++) {
      ss << "^";
    }
    ss << std::endl;

    v8::Local<v8::Value> stackTraceString;
    if (tryCatch->StackTrace(context).ToLocal(&stackTraceString) &&
        stackTraceString->IsString() &&
        v8::Local<v8::String>::Cast(stackTraceString)->Length() > 0) {
      v8::String::Utf8Value stackTrace(isolate, stackTraceString);
      ss << JSIV8ValueConverter::ToSTLString(stackTrace) << std::endl;
    }

    throw jsi::JSError(const_cast<V8Runtime &>(*this), ss.str());
    return;
  }
}

std::unique_ptr<v8::ScriptCompiler::CachedData>
V8Runtime::LoadCodeCacheIfNeeded(const std::string &codecachePath) {
  // caching is for main runtime only
  if (isSharedRuntime_) {
    return nullptr;
  }

  if (config_->codecacheMode == V8RuntimeConfig::CodecacheMode::kNone) {
    return nullptr;
  }

  if (config_->codecacheMode == V8RuntimeConfig::CodecacheMode::kPrebuilt) {
    assert(config_->prebuiltCodecacheBlob);
    return std::make_unique<v8::ScriptCompiler::CachedData>(
        reinterpret_cast<const uint8_t *>(
            config_->prebuiltCodecacheBlob->c_str()),
        static_cast<int>(config_->prebuiltCodecacheBlob->size()),
        v8::ScriptCompiler::CachedData::BufferPolicy::BufferNotOwned);
  }

  FILE *file = fopen(codecachePath.c_str(), "rb");
  if (!file) {
    LOG(INFO) << "Cannot load codecache file: " << codecachePath;
    return nullptr;
  }
  fseek(file, 0, SEEK_END);
  size_t size = ftell(file);
  uint8_t *buffer = new uint8_t[size];
  rewind(file);

  fread(buffer, size, 1, file);
  fclose(file);

  return std::make_unique<v8::ScriptCompiler::CachedData>(
      buffer,
      static_cast<int>(size),
      v8::ScriptCompiler::CachedData::BufferPolicy::BufferOwned);
}

bool V8Runtime::SaveCodeCacheIfNeeded(
    const v8::Local<v8::Script> &script,
    const std::string &codecachePath,
    v8::ScriptCompiler::CachedData *cachedData) {
  // caching is for main runtime only
  if (isSharedRuntime_) {
    return false;
  }

  if (cachedData && !cachedData->rejected) {
    return false;
  }

  if (config_->codecacheMode == V8RuntimeConfig::CodecacheMode::kNone ||
      config_->codecacheMode == V8RuntimeConfig::CodecacheMode::kPrebuilt) {
    return false;
  }

  v8::HandleScope scopedHandle(isolate_);

  v8::Local<v8::UnboundScript> unboundScript = script->GetUnboundScript();
  std::unique_ptr<v8::ScriptCompiler::CachedData> newCachedData;
  newCachedData.reset(v8::ScriptCompiler::CreateCodeCache(unboundScript));
  if (!newCachedData) {
    return false;
  }

  FILE *file = fopen(codecachePath.c_str(), "wb");
  if (!file) {
    LOG(ERROR) << "Cannot save codecache file: " << codecachePath;
    return false;
  }
  fwrite(newCachedData->data, 1, newCachedData->length, file);
  fclose(file);
  return true;
}

std::unique_ptr<v8::ScriptCompiler::Source> V8Runtime::UseFakeSourceIfNeeded(
    const v8::ScriptOrigin &origin,
    v8::ScriptCompiler::CachedData *cachedData) {
  // caching is for main runtime only
  if (isSharedRuntime_) {
    return nullptr;
  }

  if (!cachedData) {
    return nullptr;
  }

  if (config_->codecacheMode == V8RuntimeConfig::CodecacheMode::kPrebuilt ||
      config_->codecacheMode ==
          V8RuntimeConfig::CodecacheMode::kNormalWithStubBundle) {
    uint32_t payloadSize = (cachedData->data[8] << 0) |
        (cachedData->data[9] << 8) | (cachedData->data[10] << 16) |
        (cachedData->data[11] << 24);
    std::string stubScriptString(payloadSize, ' ');
    v8::Local<v8::String> stubScript =
        v8::String::NewFromUtf8(isolate_, stubScriptString.c_str())
            .ToLocalChecked();
    return std::make_unique<v8::ScriptCompiler::Source>(
        stubScript, origin, cachedData);
  }

  return nullptr;
}

// static
v8::Platform *V8Runtime::GetPlatform() {
  return s_platform.get();
}

//
// jsi::Runtime implementations
//
jsi::Value V8Runtime::evaluateJavaScript(
    const std::shared_ptr<const jsi::Buffer> &buffer,
    const std::string &sourceURL) {
  v8::Locker locker(isolate_);
  v8::Isolate::Scope scopedIsolate(isolate_);
  v8::HandleScope scopedHandle(isolate_);
  v8::Context::Scope scopedContext(context_.Get(isolate_));
  v8::Local<v8::String> string;
  if (JSIV8ValueConverter::ToV8String(*this, buffer).ToLocal(&string)) {
    return ExecuteScript(isolate_, string, sourceURL);
  }
  return {};
}

std::shared_ptr<const jsi::PreparedJavaScript> V8Runtime::prepareJavaScript(
    const std::shared_ptr<const jsi::Buffer> &buffer,
    std::string sourceURL) {
  return std::make_shared<jsi::SourceJavaScriptPreparation>(
      buffer, std::move(sourceURL));
}

jsi::Value V8Runtime::evaluatePreparedJavaScript(
    const std::shared_ptr<const jsi::PreparedJavaScript> &js) {
  assert(
      dynamic_cast<const jsi::SourceJavaScriptPreparation *>(js.get()) &&
      "preparedJavaScript must be a SourceJavaScriptPreparation");
  auto sourceJs =
      std::static_pointer_cast<const jsi::SourceJavaScriptPreparation>(js);
  return evaluateJavaScript(sourceJs, sourceJs->sourceURL());
}

bool V8Runtime::drainMicrotasks(int maxMicrotasksHint) {
  v8::Locker locker(isolate_);
  v8::Isolate::Scope scopedIsolate(isolate_);
  v8::HandleScope scopedHandle(isolate_);
  v8::Context::Scope scopedContext(context_.Get(isolate_));

  while (v8::platform::PumpMessageLoop(
      s_platform.get(),
      isolate_,
      v8::platform::MessageLoopBehavior::kDoNotWait)) {
    continue;
  }
  isolate_->PerformMicrotaskCheckpoint();
  return true;
}

jsi::Object V8Runtime::global() {
  v8::Locker locker(isolate_);
  v8::Isolate::Scope scopedIsolate(isolate_);
  v8::HandleScope scopedHandle(isolate_);
  v8::Context::Scope scopedContext(context_.Get(isolate_));

  return make<jsi::Object>(
      new V8PointerValue(isolate_, context_.Get(isolate_)->Global()));
}

std::string V8Runtime::description() {
  std::ostringstream ss;
  ss << "<V8Runtime@" << this << ">";
  return ss.str();
}

bool V8Runtime::isInspectable() {
  return !!inspectorClient_;
}

// These clone methods are shallow clone
jsi::Runtime::PointerValue *V8Runtime::cloneSymbol(
    const Runtime::PointerValue *pv) {
  if (!pv) {
    return nullptr;
  }
  v8::Locker locker(isolate_);
  v8::Isolate::Scope scopedIsolate(isolate_);
  v8::HandleScope scopedHandle(isolate_);
  v8::Context::Scope scopedContext(context_.Get(isolate_));

  const V8PointerValue *v8PointerValue =
      static_cast<const V8PointerValue *>(pv);
  assert(v8PointerValue->Get(isolate_)->IsSymbol());
  return new V8PointerValue(isolate_, v8PointerValue->Get(isolate_));
}

jsi::Runtime::PointerValue *V8Runtime::cloneString(
    const Runtime::PointerValue *pv) {
  if (!pv) {
    return nullptr;
  }

  v8::Locker locker(isolate_);
  v8::Isolate::Scope scopedIsolate(isolate_);
  v8::HandleScope scopedHandle(isolate_);
  v8::Context::Scope scopedContext(context_.Get(isolate_));

  const V8PointerValue *v8PointerValue =
      static_cast<const V8PointerValue *>(pv);
  assert(v8PointerValue->Get(isolate_)->IsString());
  return new V8PointerValue(isolate_, v8PointerValue->Get(isolate_));
}

jsi::Runtime::PointerValue *V8Runtime::cloneObject(
    const Runtime::PointerValue *pv) {
  if (!pv) {
    return nullptr;
  }

  v8::Locker locker(isolate_);
  v8::Isolate::Scope scopedIsolate(isolate_);
  v8::HandleScope scopedHandle(isolate_);
  v8::Context::Scope scopedContext(context_.Get(isolate_));

  const V8PointerValue *v8PointerValue =
      static_cast<const V8PointerValue *>(pv);
  assert(v8PointerValue->Get(isolate_)->IsObject());
  return new V8PointerValue(isolate_, v8PointerValue->Get(isolate_));
}

jsi::Runtime::PointerValue *V8Runtime::clonePropNameID(
    const Runtime::PointerValue *pv) {
  return cloneString(pv);
}

jsi::PropNameID V8Runtime::createPropNameIDFromAscii(
    const char *str,
    size_t length) {
  v8::Locker locker(isolate_);
  v8::Isolate::Scope scopedIsolate(isolate_);
  v8::HandleScope scopedHandle(isolate_);
  v8::Context::Scope scopedContext(context_.Get(isolate_));
  V8PointerValue *value =
      V8PointerValue::createFromOneByte(isolate_, str, length);
  if (!value) {
    throw jsi::JSError(*this, "createFromOneByte() - string creation failed.");
  }

  return make<jsi::PropNameID>(value);
}

jsi::PropNameID V8Runtime::createPropNameIDFromUtf8(
    const uint8_t *utf8,
    size_t length) {
  v8::Locker locker(isolate_);
  v8::Isolate::Scope scopedIsolate(isolate_);
  v8::HandleScope scopedHandle(isolate_);
  v8::Context::Scope scopedContext(context_.Get(isolate_));
  V8PointerValue *value =
      V8PointerValue::createFromUtf8(isolate_, utf8, length);
  if (!value) {
    throw jsi::JSError(*this, "createFromUtf8() - string creation failed.");
  }

  return make<jsi::PropNameID>(value);
}

jsi::PropNameID V8Runtime::createPropNameIDFromString(const jsi::String &str) {
  v8::Locker locker(isolate_);
  v8::Isolate::Scope scopedIsolate(isolate_);
  v8::HandleScope scopedHandle(isolate_);
  v8::Context::Scope scopedContext(context_.Get(isolate_));

  const V8PointerValue *v8PointerValue =
      static_cast<const V8PointerValue *>(getPointerValue(str));
  assert(v8PointerValue->Get(isolate_)->IsString());

  v8::String::Utf8Value utf8(isolate_, v8PointerValue->Get(isolate_));
  return createPropNameIDFromUtf8(
      reinterpret_cast<const uint8_t *>(*utf8), utf8.length());
}

std::string V8Runtime::utf8(const jsi::PropNameID &sym) {
  v8::Locker locker(isolate_);
  v8::Isolate::Scope scopedIsolate(isolate_);
  v8::HandleScope scopedHandle(isolate_);
  v8::Context::Scope scopedContext(context_.Get(isolate_));

  const V8PointerValue *v8PointerValue =
      static_cast<const V8PointerValue *>(getPointerValue(sym));
  v8::String::Utf8Value utf8(isolate_, v8PointerValue->Get(isolate_));
  return JSIV8ValueConverter::ToSTLString(utf8);
}

bool V8Runtime::compare(const jsi::PropNameID &a, const jsi::PropNameID &b) {
  v8::Locker locker(isolate_);
  v8::Isolate::Scope scopedIsolate(isolate_);
  v8::HandleScope scopedHandle(isolate_);
  v8::Context::Scope scopedContext(context_.Get(isolate_));

  const V8PointerValue *v8PointerValueA =
      static_cast<const V8PointerValue *>(getPointerValue(a));
  const V8PointerValue *v8PointerValueB =
      static_cast<const V8PointerValue *>(getPointerValue(b));

  v8::Local<v8::String> v8StringA =
      v8::Local<v8::String>::Cast(v8PointerValueA->Get(isolate_));
  v8::Local<v8::String> v8StringB =
      v8::Local<v8::String>::Cast(v8PointerValueB->Get(isolate_));
  return v8StringA->StringEquals(v8StringB);
}

std::string V8Runtime::symbolToString(const jsi::Symbol &symbol) {
  return jsi::Value(*this, symbol).toString(*this).utf8(*this);
}

jsi::String V8Runtime::createStringFromAscii(const char *str, size_t length) {
  v8::Locker locker(isolate_);
  v8::Isolate::Scope scopedIsolate(isolate_);
  v8::HandleScope scopedHandle(isolate_);
  v8::Context::Scope scopedContext(context_.Get(isolate_));

  V8PointerValue *value =
      V8PointerValue::createFromOneByte(isolate_, str, length);
  if (!value) {
    throw jsi::JSError(*this, "createFromOneByte() - string creation failed.");
  }

  return make<jsi::String>(value);
}

jsi::String V8Runtime::createStringFromUtf8(const uint8_t *str, size_t length) {
  v8::Locker locker(isolate_);
  v8::Isolate::Scope scopedIsolate(isolate_);
  v8::HandleScope scopedHandle(isolate_);
  v8::Context::Scope scopedContext(context_.Get(isolate_));

  V8PointerValue *value = V8PointerValue::createFromUtf8(isolate_, str, length);
  if (!value) {
    throw jsi::JSError(*this, "createFromUtf8() - string creation failed.");
  }

  return make<jsi::String>(value);
}

std::string V8Runtime::utf8(const jsi::String &str) {
  v8::Locker locker(isolate_);
  v8::Isolate::Scope scopedIsolate(isolate_);
  v8::HandleScope scopedHandle(isolate_);
  v8::Context::Scope scopedContext(context_.Get(isolate_));

  const V8PointerValue *v8PointerValue =
      static_cast<const V8PointerValue *>(getPointerValue(str));
  assert(v8PointerValue->Get(isolate_)->IsString());

  v8::String::Utf8Value utf8(isolate_, v8PointerValue->Get(isolate_));
  return JSIV8ValueConverter::ToSTLString(utf8);
}

jsi::Object V8Runtime::createObject() {
  v8::Locker locker(isolate_);
  v8::Isolate::Scope scopedIsolate(isolate_);
  v8::HandleScope scopedHandle(isolate_);
  v8::Context::Scope scopedContext(context_.Get(isolate_));

  v8::Local<v8::Object> object = v8::Object::New(isolate_);
  return make<jsi::Object>(new V8PointerValue(isolate_, object));
}

jsi::Object V8Runtime::createObject(
    std::shared_ptr<jsi::HostObject> hostObject) {
  v8::Locker locker(isolate_);
  v8::Isolate::Scope scopedIsolate(isolate_);
  v8::HandleScope scopedHandle(isolate_);
  v8::Context::Scope scopedContext(context_.Get(isolate_));

  HostObjectProxy *hostObjectProxy =
      new HostObjectProxy(*this, isolate_, hostObject);
  v8::Local<v8::Object> v8Object;

  v8::Local<v8::ObjectTemplate> hostObjectTemplate =
      v8::ObjectTemplate::New(isolate_);
  hostObjectTemplate->SetHandler(v8::NamedPropertyHandlerConfiguration(
      HostObjectProxy::Getter,
      HostObjectProxy::Setter,
      nullptr,
      nullptr,
      HostObjectProxy::Enumerator));
  hostObjectTemplate->SetInternalFieldCount(1);

  if (!hostObjectTemplate->NewInstance(isolate_->GetCurrentContext())
           .ToLocal(&v8Object)) {
    delete hostObjectProxy;
    throw jsi::JSError(*this, "Unable to create HostObject");
  }

  v8::Local<v8::External> wrappedHostObjectProxy =
      v8::External::New(isolate_, hostObjectProxy);
  v8Object->SetInternalField(0, wrappedHostObjectProxy);
  hostObjectProxy->BindFinalizer(v8Object);

  return make<jsi::Object>(new V8PointerValue(isolate_, v8Object));
}

std::shared_ptr<jsi::HostObject> V8Runtime::getHostObject(
    const jsi::Object &object) {
  assert(isHostObject(object));

  // We are guarenteed at this point to have isHostObject(obj) == true
  // so the internal data should be HostObjectMetadata
  v8::Locker locker(isolate_);
  v8::Isolate::Scope scopedIsolate(isolate_);
  v8::HandleScope scopedHandle(isolate_);
  v8::Context::Scope scopedContext(context_.Get(isolate_));

  v8::Local<v8::Object> v8Object =
      JSIV8ValueConverter::ToV8Object(*this, object);
  v8::Local<v8::External> internalField =
      v8::Local<v8::External>::Cast(v8Object->GetInternalField(0));
  HostObjectProxy *hostObjectProxy =
      reinterpret_cast<HostObjectProxy *>(internalField->Value());
  assert(hostObjectProxy);
  return hostObjectProxy->GetHostObject();
}

jsi::HostFunctionType &V8Runtime::getHostFunction(
    const jsi::Function &function) {
  assert(isHostFunction(function));

  // We know that isHostFunction(function) is true here, so its safe to proceed
  v8::Locker locker(isolate_);
  v8::Isolate::Scope scopedIsolate(isolate_);
  v8::HandleScope scopedHandle(isolate_);
  v8::Context::Scope scopedContext(context_.Get(isolate_));

  const V8PointerValue *v8PointerValue =
      static_cast<const V8PointerValue *>(getPointerValue(function));
  assert(v8PointerValue->Get(isolate_)->IsFunction());
  v8::Local<v8::Function> v8Function =
      v8::Local<v8::Function>::Cast(v8PointerValue->Get(isolate_));

  v8::Local<v8::String> prop =
      v8::String::NewFromUtf8(
          isolate_, kHostFunctionProxyProp, v8::NewStringType::kNormal)
          .ToLocalChecked();
  v8::Local<v8::External> wrappedHostFunctionProxy =
      v8::Local<v8::External>::Cast(
          v8Function->Get(isolate_->GetCurrentContext(), prop)
              .ToLocalChecked());
  HostFunctionProxy *hostFunctionProxy =
      reinterpret_cast<HostFunctionProxy *>(wrappedHostFunctionProxy->Value());
  assert(hostFunctionProxy);
  return hostFunctionProxy->GetHostFunction();
}

jsi::Value V8Runtime::getProperty(
    const jsi::Object &object,
    const jsi::PropNameID &name) {
  v8::Locker locker(isolate_);
  v8::Isolate::Scope scopedIsolate(isolate_);
  v8::HandleScope scopedHandle(isolate_);
  v8::Context::Scope scopedContext(context_.Get(isolate_));

  v8::TryCatch tryCatch(isolate_);
  v8::Local<v8::Object> v8Object =
      JSIV8ValueConverter::ToV8Object(*this, object);

  v8::MaybeLocal<v8::Value> result = v8Object->Get(
      isolate_->GetCurrentContext(),
      JSIV8ValueConverter::ToV8String(*this, name));
  if (tryCatch.HasCaught()) {
    ReportException(isolate_, &tryCatch);
  }

  if (result.IsEmpty()) {
    return jsi::Value::undefined();
  }
  return JSIV8ValueConverter::ToJSIValue(isolate_, result.ToLocalChecked());
}

jsi::Value V8Runtime::getProperty(
    const jsi::Object &object,
    const jsi::String &name) {
  v8::Locker locker(isolate_);
  v8::Isolate::Scope scopedIsolate(isolate_);
  v8::HandleScope scopedHandle(isolate_);
  v8::Context::Scope scopedContext(context_.Get(isolate_));

  v8::TryCatch tryCatch(isolate_);
  v8::Local<v8::Object> v8Object =
      JSIV8ValueConverter::ToV8Object(*this, object);

  v8::MaybeLocal<v8::Value> result = v8Object->Get(
      isolate_->GetCurrentContext(),
      JSIV8ValueConverter::ToV8String(*this, name));
  if (tryCatch.HasCaught()) {
    ReportException(isolate_, &tryCatch);
  }
  if (result.IsEmpty()) {
    return jsi::Value::undefined();
  }
  return JSIV8ValueConverter::ToJSIValue(isolate_, result.ToLocalChecked());
}

bool V8Runtime::hasProperty(
    const jsi::Object &object,
    const jsi::PropNameID &name) {
  v8::Locker locker(isolate_);
  v8::Isolate::Scope scopedIsolate(isolate_);
  v8::HandleScope scopedHandle(isolate_);
  v8::Context::Scope scopedContext(context_.Get(isolate_));

  v8::TryCatch tryCatch(isolate_);
  v8::Local<v8::Object> v8Object =
      JSIV8ValueConverter::ToV8Object(*this, object);

  v8::Maybe<bool> result = v8Object->Has(
      isolate_->GetCurrentContext(),
      JSIV8ValueConverter::ToV8String(*this, name));
  if (tryCatch.HasCaught()) {
    ReportException(isolate_, &tryCatch);
  }
  if (result.IsNothing()) {
    return false;
    // throw jsi::JSError(*this, "V8Runtime::hasProperty failed.");
  }
  return result.FromJust();
}

bool V8Runtime::hasProperty(
    const jsi::Object &object,
    const jsi::String &name) {
  v8::Locker locker(isolate_);
  v8::Isolate::Scope scopedIsolate(isolate_);
  v8::HandleScope scopedHandle(isolate_);
  v8::Context::Scope scopedContext(context_.Get(isolate_));

  v8::TryCatch tryCatch(isolate_);
  v8::Local<v8::Object> v8Object =
      JSIV8ValueConverter::ToV8Object(*this, object);

  v8::Maybe<bool> result = v8Object->Has(
      isolate_->GetCurrentContext(),
      JSIV8ValueConverter::ToV8String(*this, name));
  if (tryCatch.HasCaught()) {
    ReportException(isolate_, &tryCatch);
  }
  if (result.IsNothing()) {
    return false;
    // throw jsi::JSError(*this, "V8Runtime::hasProperty failed.");
  }
  return result.FromJust();
}

void V8Runtime::setPropertyValue(
    jsi::Object &object,
    const jsi::PropNameID &name,
    const jsi::Value &value) {
  v8::Locker locker(isolate_);
  v8::Isolate::Scope scopedIsolate(isolate_);
  v8::HandleScope scopedHandle(isolate_);
  v8::Context::Scope scopedContext(context_.Get(isolate_));

  v8::Local<v8::Object> v8Object =
      JSIV8ValueConverter::ToV8Object(*this, object);

  if (v8Object
          ->Set(
              isolate_->GetCurrentContext(),
              JSIV8ValueConverter::ToV8String(*this, name),
              JSIV8ValueConverter::ToV8Value(*this, value))
          .IsNothing()) {
    throw jsi::JSError(*this, "V8Runtime::setPropertyValue failed.");
  }
}

void V8Runtime::setPropertyValue(
    jsi::Object &object,
    const jsi::String &name,
    const jsi::Value &value) {
  v8::Locker locker(isolate_);
  v8::Isolate::Scope scopedIsolate(isolate_);
  v8::HandleScope scopedHandle(isolate_);
  v8::Context::Scope scopedContext(context_.Get(isolate_));

  v8::Local<v8::Object> v8Object =
      JSIV8ValueConverter::ToV8Object(*this, object);

  if (v8Object
          ->Set(
              isolate_->GetCurrentContext(),
              JSIV8ValueConverter::ToV8String(*this, name),
              JSIV8ValueConverter::ToV8Value(*this, value))
          .IsNothing()) {
    throw jsi::JSError(*this, "V8Runtime::setPropertyValue failed.");
  }
}

bool V8Runtime::isArray(const jsi::Object &object) const {
  v8::Locker locker(isolate_);
  v8::Isolate::Scope scopedIsolate(isolate_);
  v8::HandleScope scopedHandle(isolate_);
  v8::Context::Scope scopedContext(context_.Get(isolate_));

  v8::Local<v8::Object> v8Object =
      JSIV8ValueConverter::ToV8Object(*this, object);
  return v8Object->IsArray();
}

bool V8Runtime::isArrayBuffer(const jsi::Object &object) const {
  v8::Locker locker(isolate_);
  v8::Isolate::Scope scopedIsolate(isolate_);
  v8::HandleScope scopedHandle(isolate_);
  v8::Context::Scope scopedContext(context_.Get(isolate_));

  v8::Local<v8::Object> v8Object =
      JSIV8ValueConverter::ToV8Object(*this, object);
  return v8Object->IsArrayBuffer();
}

bool V8Runtime::isFunction(const jsi::Object &object) const {
  v8::Locker locker(isolate_);
  v8::Isolate::Scope scopedIsolate(isolate_);
  v8::HandleScope scopedHandle(isolate_);
  v8::Context::Scope scopedContext(context_.Get(isolate_));

  v8::Local<v8::Object> v8Object =
      JSIV8ValueConverter::ToV8Object(*this, object);
  return v8Object->IsFunction();
}

bool V8Runtime::isHostObject(const jsi::Object &object) const {
  v8::Locker locker(isolate_);
  v8::Isolate::Scope scopedIsolate(isolate_);
  v8::HandleScope scopedHandle(isolate_);
  v8::Context::Scope scopedContext(context_.Get(isolate_));

  v8::Local<v8::Object> v8Object =
      JSIV8ValueConverter::ToV8Object(*this, object);
  return v8Object->InternalFieldCount() == 1;
}

bool V8Runtime::isHostFunction(const jsi::Function &function) const {
  v8::Locker locker(isolate_);
  v8::Isolate::Scope scopedIsolate(isolate_);
  v8::HandleScope scopedHandle(isolate_);
  v8::Context::Scope scopedContext(context_.Get(isolate_));

  v8::Local<v8::Function> v8Function =
      JSIV8ValueConverter::ToV8Function(*this, function);

  v8::Local<v8::String> prop =
      v8::String::NewFromUtf8(
          isolate_, kHostFunctionProxyProp, v8::NewStringType::kNormal)
          .ToLocalChecked();
  return v8Function->Has(isolate_->GetCurrentContext(), prop).ToChecked();
}

jsi::Array V8Runtime::getPropertyNames(const jsi::Object &object) {
  v8::Locker locker(isolate_);
  v8::Isolate::Scope scopedIsolate(isolate_);
  v8::HandleScope scopedHandle(isolate_);
  v8::Context::Scope scopedContext(context_.Get(isolate_));

  v8::Local<v8::Object> v8Object =
      JSIV8ValueConverter::ToV8Object(*this, object);
  v8::Local<v8::Array> propertyNames;
  if (!v8Object
           ->GetPropertyNames(
               isolate_->GetCurrentContext(),
               v8::KeyCollectionMode::kIncludePrototypes,
               static_cast<v8::PropertyFilter>(
                   v8::ONLY_ENUMERABLE | v8::SKIP_SYMBOLS),
               v8::IndexFilter::kIncludeIndices,
               v8::KeyConversionMode::kConvertToString)
           .ToLocal(&propertyNames)) {
    std::abort();
  }
  return make<jsi::Object>(new V8PointerValue(isolate_, propertyNames))
      .getArray(*this);
}

jsi::WeakObject V8Runtime::createWeakObject(const jsi::Object &weakObject) {
  v8::Locker locker(isolate_);
  v8::Isolate::Scope scopedIsolate(isolate_);
  v8::HandleScope scopedHandle(isolate_);
  v8::Context::Scope scopedContext(context_.Get(isolate_));

  const V8PointerValue *v8PointerValue =
      static_cast<const V8PointerValue *>(getPointerValue(weakObject));
  assert(v8PointerValue->Get(isolate_)->IsObject());

  v8::Global<v8::Value> weakRef =
      v8::Global<v8::Value>(isolate_, v8PointerValue->Get(isolate_));
  weakRef.SetWeak();
  return make<jsi::WeakObject>(
      new V8PointerValue(isolate_, std::move(weakRef)));
}

jsi::Value V8Runtime::lockWeakObject(jsi::WeakObject &weakObject) {
  v8::Locker locker(isolate_);
  v8::Isolate::Scope scopedIsolate(isolate_);
  v8::HandleScope scopedHandle(isolate_);
  v8::Context::Scope scopedContext(context_.Get(isolate_));

  const V8PointerValue *v8PointerValue =
      static_cast<const V8PointerValue *>(getPointerValue(weakObject));
  assert(v8PointerValue->Get(isolate_)->IsObject());
  return JSIV8ValueConverter::ToJSIValue(
      isolate_, v8PointerValue->Get(isolate_));
}

jsi::Array V8Runtime::createArray(size_t length) {
  v8::Locker locker(isolate_);
  v8::Isolate::Scope scopedIsolate(isolate_);
  v8::HandleScope scopedHandle(isolate_);
  v8::Context::Scope scopedContext(context_.Get(isolate_));

  v8::Local<v8::Array> v8Array =
      v8::Array::New(isolate_, static_cast<int>(length));
  return make<jsi::Object>(new V8PointerValue(isolate_, v8Array))
      .getArray(*this);
}

size_t V8Runtime::size(const jsi::Array &array) {
  v8::Locker locker(isolate_);
  v8::Isolate::Scope scopedIsolate(isolate_);
  v8::HandleScope scopedHandle(isolate_);
  v8::Context::Scope scopedContext(context_.Get(isolate_));

  v8::Local<v8::Array> v8Array = JSIV8ValueConverter::ToV8Array(*this, array);
  return v8Array->Length();
}

size_t V8Runtime::size(const jsi::ArrayBuffer &arrayBuffer) {
  v8::Locker locker(isolate_);
  v8::Isolate::Scope scopedIsolate(isolate_);
  v8::HandleScope scopedHandle(isolate_);
  v8::Context::Scope scopedContext(context_.Get(isolate_));

  v8::Local<v8::Object> v8Object =
      JSIV8ValueConverter::ToV8Object(*this, arrayBuffer);
  assert(v8Object->IsArrayBuffer());
  v8::ArrayBuffer *v8ArrayBuffer = v8::ArrayBuffer::Cast(*v8Object);
  return v8ArrayBuffer->ByteLength();
}

uint8_t *V8Runtime::data(const jsi::ArrayBuffer &arrayBuffer) {
  v8::Locker locker(isolate_);
  v8::Isolate::Scope scopedIsolate(isolate_);
  v8::HandleScope scopedHandle(isolate_);
  v8::Context::Scope scopedContext(context_.Get(isolate_));

  v8::Local<v8::Object> v8Object =
      JSIV8ValueConverter::ToV8Object(*this, arrayBuffer);
  assert(v8Object->IsArrayBuffer());
  v8::ArrayBuffer *v8ArrayBuffer = v8::ArrayBuffer::Cast(*v8Object);
  return reinterpret_cast<uint8_t *>(v8ArrayBuffer->GetBackingStore()->Data());
}

jsi::Value V8Runtime::getValueAtIndex(const jsi::Array &array, size_t i) {
  v8::Locker locker(isolate_);
  v8::Isolate::Scope scopedIsolate(isolate_);
  v8::HandleScope scopedHandle(isolate_);
  v8::Context::Scope scopedContext(context_.Get(isolate_));

  v8::Local<v8::Array> v8Array = JSIV8ValueConverter::ToV8Array(*this, array);
  v8::MaybeLocal<v8::Value> result =
      v8Array->Get(isolate_->GetCurrentContext(), static_cast<uint32_t>(i));
  if (result.IsEmpty()) {
    throw jsi::JSError(*this, "V8Runtime::getValueAtIndex failed.");
  }
  return JSIV8ValueConverter::ToJSIValue(isolate_, result.ToLocalChecked());
}

void V8Runtime::setValueAtIndexImpl(
    jsi::Array &array,
    size_t i,
    const jsi::Value &value) {
  v8::Locker locker(isolate_);
  v8::Isolate::Scope scopedIsolate(isolate_);
  v8::HandleScope scopedHandle(isolate_);
  v8::Context::Scope scopedContext(context_.Get(isolate_));

  v8::Local<v8::Array> v8Array = JSIV8ValueConverter::ToV8Array(*this, array);
  v8::Maybe<bool> result = v8Array->Set(
      isolate_->GetCurrentContext(),
      static_cast<uint32_t>(i),
      JSIV8ValueConverter::ToV8Value(*this, value));
  if (result.IsNothing()) {
    throw jsi::JSError(*this, "V8Runtime::setValueAtIndexImpl failed.");
  }
}

jsi::Function V8Runtime::createFunctionFromHostFunction(
    const jsi::PropNameID &name,
    unsigned int paramCount,
    jsi::HostFunctionType func) {
  v8::Locker locker(isolate_);
  v8::Isolate::Scope scopedIsolate(isolate_);
  v8::HandleScope scopedHandle(isolate_);
  v8::Context::Scope scopedContext(context_.Get(isolate_));

  HostFunctionProxy *hostFunctionProxy =
      new HostFunctionProxy(*this, isolate_, std::move(func));
  v8::Local<v8::External> wrappedHostFunctionProxy =
      v8::External::New(isolate_, hostFunctionProxy);
  v8::Local<v8::Function> v8HostFunction =
      v8::Function::New(
          isolate_->GetCurrentContext(),
          HostFunctionProxy::FunctionCallback,
          wrappedHostFunctionProxy)
          .ToLocalChecked();
  hostFunctionProxy->BindFinalizer(v8HostFunction);

  v8::Local<v8::Function> v8FunctionContainer =
      v8::Function::New(
          isolate_->GetCurrentContext(),
          V8Runtime::OnHostFuncionContainerCallback,
          v8HostFunction)
          .ToLocalChecked();

  v8::Local<v8::String> prop =
      v8::String::NewFromUtf8(
          isolate_, kHostFunctionProxyProp, v8::NewStringType::kNormal)
          .ToLocalChecked();
  v8FunctionContainer
      ->Set(isolate_->GetCurrentContext(), prop, wrappedHostFunctionProxy)
      .Check();
  // wrappedHostFunctionProxy is mainly owned by v8HostFunction and not bind
  // finalizer to v8FunctionContainer

  v8::Local<v8::String> v8Name = JSIV8ValueConverter::ToV8String(*this, name);
  v8FunctionContainer->SetName(v8Name);

  return make<jsi::Object>(new V8PointerValue(isolate_, v8FunctionContainer))
      .getFunction(*this);
}

jsi::Value V8Runtime::call(
    const jsi::Function &function,
    const jsi::Value &jsThis,
    const jsi::Value *args,
    size_t count) {
  v8::Locker locker(isolate_);
  v8::Isolate::Scope scopedIsolate(isolate_);
  v8::HandleScope scopedHandle(isolate_);
  v8::Context::Scope scopedContext(context_.Get(isolate_));

  v8::TryCatch tryCatch(isolate_);
  v8::Local<v8::Function> v8Function =
      JSIV8ValueConverter::ToV8Function(*this, function);

  v8::Local<v8::Value> v8Receiver;
  if (jsThis.isUndefined()) {
    v8Receiver = context_.Get(isolate_)->Global();
  } else {
    v8Receiver = JSIV8ValueConverter::ToV8Value(*this, jsThis);
  }

  std::vector<v8::Local<v8::Value>> argv;
  for (size_t i = 0; i < count; ++i) {
    v8::Local<v8::Value> v8ArgValue =
        JSIV8ValueConverter::ToV8Value(*this, args[i]);
    argv.push_back(v8ArgValue);
  }

  v8::MaybeLocal<v8::Value> result = v8Function->Call(
      isolate_->GetCurrentContext(),
      v8Receiver,
      static_cast<int>(count),
      argv.data());

  if (tryCatch.HasCaught()) {
    ReportException(isolate_, &tryCatch);
  }

  if (result.IsEmpty()) {
    return JSIV8ValueConverter::ToJSIValue(isolate_, v8::Undefined(isolate_));
  } else {
    return JSIV8ValueConverter::ToJSIValue(isolate_, result.ToLocalChecked());
  }
}

jsi::Value V8Runtime::callAsConstructor(
    const jsi::Function &function,
    const jsi::Value *args,
    size_t count) {
  v8::Locker locker(isolate_);
  v8::Isolate::Scope scopedIsolate(isolate_);
  v8::HandleScope scopedHandle(isolate_);
  v8::Context::Scope scopedContext(context_.Get(isolate_));

  v8::TryCatch tryCatch(isolate_);
  v8::Local<v8::Function> v8Function =
      JSIV8ValueConverter::ToV8Function(*this, function);
  std::vector<v8::Local<v8::Value>> argv;
  for (size_t i = 0; i < count; i++) {
    v8::Local<v8::Value> v8ArgValue =
        JSIV8ValueConverter::ToV8Value(*this, args[i]);
    argv.push_back(v8ArgValue);
  }

  v8::Local<v8::Object> v8Object;
  if (!v8Function
           ->NewInstance(
               isolate_->GetCurrentContext(),
               static_cast<int>(count),
               argv.data())
           .ToLocal(&v8Object)) {
    throw jsi::JSError(*this, "CallAsConstructor failed");
  }

  if (tryCatch.HasCaught()) {
    ReportException(isolate_, &tryCatch);
  }

  return JSIV8ValueConverter::ToJSIValue(isolate_, v8Object);
}

bool V8Runtime::strictEquals(const jsi::Symbol &a, const jsi::Symbol &b) const {
  v8::Locker locker(isolate_);
  v8::Isolate::Scope scopedIsolate(isolate_);
  v8::HandleScope scopedHandle(isolate_);
  v8::Context::Scope scopedContext(context_.Get(isolate_));

  v8::TryCatch tryCatch(isolate_);
  v8::Local<v8::Symbol> v8SymbolA = JSIV8ValueConverter::ToV8Symbol(*this, a);
  v8::Local<v8::Symbol> v8SymbolB = JSIV8ValueConverter::ToV8Symbol(*this, b);
  bool result = v8SymbolA->StrictEquals(v8SymbolB);

  if (tryCatch.HasCaught()) {
    ReportException(isolate_, &tryCatch);
  }
  return result;
}

bool V8Runtime::strictEquals(const jsi::String &a, const jsi::String &b) const {
  v8::Locker locker(isolate_);
  v8::Isolate::Scope scopedIsolate(isolate_);
  v8::HandleScope scopedHandle(isolate_);
  v8::Context::Scope scopedContext(context_.Get(isolate_));

  v8::TryCatch tryCatch(isolate_);
  v8::Local<v8::String> v8StringA = JSIV8ValueConverter::ToV8String(*this, a);
  v8::Local<v8::String> v8StringB = JSIV8ValueConverter::ToV8String(*this, b);
  bool result = v8StringA->StrictEquals(v8StringB);

  if (tryCatch.HasCaught()) {
    ReportException(isolate_, &tryCatch);
  }
  return result;
}

bool V8Runtime::strictEquals(const jsi::Object &a, const jsi::Object &b) const {
  v8::Locker locker(isolate_);
  v8::Isolate::Scope scopedIsolate(isolate_);
  v8::HandleScope scopedHandle(isolate_);
  v8::Context::Scope scopedContext(context_.Get(isolate_));

  v8::TryCatch tryCatch(isolate_);
  v8::Local<v8::Object> v8ObjectA = JSIV8ValueConverter::ToV8Object(*this, a);
  v8::Local<v8::Object> v8ObjectB = JSIV8ValueConverter::ToV8Object(*this, b);
  bool result = v8ObjectA->StrictEquals(v8ObjectB);

  if (tryCatch.HasCaught()) {
    ReportException(isolate_, &tryCatch);
  }
  return result;
}

bool V8Runtime::instanceOf(const jsi::Object &o, const jsi::Function &f) {
  v8::Locker locker(isolate_);
  v8::Isolate::Scope scopedIsolate(isolate_);
  v8::HandleScope scopedHandle(isolate_);
  v8::Context::Scope scopedContext(context_.Get(isolate_));

  v8::TryCatch tryCatch(isolate_);
  v8::Local<v8::Object> v8Object = JSIV8ValueConverter::ToV8Object(*this, o);
  v8::Local<v8::Object> v8Function = JSIV8ValueConverter::ToV8Object(*this, f);
  bool result = v8Object->InstanceOf(isolate_->GetCurrentContext(), v8Function)
                    .ToChecked();

  if (tryCatch.HasCaught()) {
    ReportException(isolate_, &tryCatch);
  }
  return result;
}

//
// JS function/object handler callbacks
//

// static
void V8Runtime::GetRuntimeInfo(
    const v8::FunctionCallbackInfo<v8::Value> &args) {
  v8::Isolate *isolate = args.GetIsolate();
  v8::HandleScope scopedHandle(isolate);
  v8::Local<v8::Object> runtimeInfo = v8::Object::New(isolate);
  v8::Local<v8::Context> context(isolate->GetCurrentContext());

  v8::Local<v8::String> versionKey =
      v8::String::NewFromUtf8(isolate, "version", v8::NewStringType::kNormal)
          .ToLocalChecked();
  v8::Local<v8::String> versionValue =
      v8::String::NewFromUtf8(
          args.GetIsolate(), v8::V8::GetVersion(), v8::NewStringType::kNormal)
          .ToLocalChecked();
  runtimeInfo->Set(context, versionKey, versionValue).Check();

  v8::Local<v8::String> memoryKey =
      v8::String::NewFromUtf8(isolate, "memory", v8::NewStringType::kNormal)
          .ToLocalChecked();
  v8::Local<v8::Object> memoryInfo = v8::Object::New(isolate);
  v8::HeapStatistics heapStats;
  isolate->GetHeapStatistics(&heapStats);
  memoryInfo
      ->Set(
          context,
          v8::String::NewFromUtf8(
              isolate, "jsHeapSizeLimit", v8::NewStringType::kNormal)
              .ToLocalChecked(),
          v8::Number::New(isolate, heapStats.heap_size_limit()))
      .Check();
  memoryInfo
      ->Set(
          context,
          v8::String::NewFromUtf8(
              isolate, "totalJSHeapSize", v8::NewStringType::kNormal)
              .ToLocalChecked(),
          v8::Number::New(isolate, heapStats.total_heap_size()))
      .Check();
  memoryInfo
      ->Set(
          context,
          v8::String::NewFromUtf8(
              isolate, "usedJSHeapSize", v8::NewStringType::kNormal)
              .ToLocalChecked(),
          v8::Number::New(isolate, heapStats.used_heap_size()))
      .Check();
  runtimeInfo->Set(context, memoryKey, memoryInfo).Check();

  args.GetReturnValue().Set(runtimeInfo);
}

// static
void V8Runtime::OnHostFuncionContainerCallback(
    const v8::FunctionCallbackInfo<v8::Value> &args) {
  v8::HandleScope scopedHandle(args.GetIsolate());

  v8::Local<v8::Function> v8HostFunction =
      v8::Local<v8::Function>::Cast(args.Data());
  std::vector<v8::Local<v8::Value>> argv;
  for (size_t i = 0; i < args.Length(); ++i) {
    argv.push_back(args[i]);
  }
  v8::MaybeLocal<v8::Value> result = v8HostFunction->Call(
      args.GetIsolate()->GetCurrentContext(),
      args.This(),
      args.Length(),
      argv.data());

  if (!result.IsEmpty()) {
    args.GetReturnValue().Set(result.ToLocalChecked());
  }
}

} // namespace rnv8
