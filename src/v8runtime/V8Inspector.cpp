/*
 * Copyright (c) Kudo Chien.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include "V8Inspector.h"

#include <regex>
#include <sstream>
#include <thread>

#include "folly/dynamic.h"
#include "folly/json.h"
#include "v8.h"

namespace react = facebook::react;

namespace rnv8 {

static const char kInspectorName[] = "React Native V8 Inspector";

namespace {
std::string ToSTLString(
    v8::Isolate *isolate,
    const v8_inspector::StringView &stringView) {
  int length = static_cast<int>(stringView.length());
  v8::MaybeLocal<v8::String> v8StringNullable =
      (stringView.is8Bit()
           ? v8::String::NewFromOneByte(
                 isolate,
                 reinterpret_cast<const uint8_t *>(stringView.characters8()),
                 v8::NewStringType::kNormal,
                 length)
           : v8::String::NewFromTwoByte(
                 isolate,
                 reinterpret_cast<const uint16_t *>(stringView.characters16()),
                 v8::NewStringType::kNormal,
                 length));
  if (v8StringNullable.IsEmpty()) {
    return std::string();
  }
  v8::Local<v8::String> v8String = v8StringNullable.ToLocalChecked();
  v8::String::Utf8Value utf8V8String(isolate, v8String);
  if (*utf8V8String)
    return std::string(*utf8V8String, utf8V8String.length());
  else
    return std::string();
}

v8_inspector::StringView ToStringView(const std::string &string) {
  return v8_inspector::StringView(
      reinterpret_cast<const uint8_t *>(string.data()), string.size());
}

std::string stripMetroCachePrevention(const std::string &message) {
  std::string result;
  try {
    auto messageObj = folly::parseJson(message);
    auto method = messageObj["method"].asString();
    if (method == "Debugger.setBreakpointByUrl") {
      std::regex regex("&?cachePrevention=[0-9]*");
      auto params = messageObj["params"];
      auto urlPtr = params.get_ptr("url");
      if (urlPtr) {
        messageObj["params"]["url"] =
            std::regex_replace(urlPtr->asString(), regex, "");
      }
      auto urlRegexPtr = params.get_ptr("urlRegex");
      if (urlRegexPtr) {
        messageObj["params"]["urlRegex"] =
            std::regex_replace(urlRegexPtr->asString(), regex, "");
      }
    }
    result = folly::toJson(messageObj);
  } catch (...) {
    result = message;
  }
  return result;
}

} // namespace

InspectorFrontend::InspectorFrontend(
    InspectorClient *client,
    v8::Local<v8::Context> context)
    : client_(client) {
  isolate_ = context->GetIsolate();
}

void InspectorFrontend::sendResponse(
    int callId,
    std::unique_ptr<v8_inspector::StringBuffer> message) {
  client_->SendRemoteMessage(message->string());
}

void InspectorFrontend::sendNotification(
    std::unique_ptr<v8_inspector::StringBuffer> message) {
  client_->SendRemoteMessage(message->string());
}

class LocalConnection : public react::ILocalConnection {
 public:
  LocalConnection(std::weak_ptr<InspectorClient> weakClient)
      : weakClient_(weakClient) {}
  ~LocalConnection() override {}

  void sendMessage(std::string message) override {
    auto client = weakClient_.lock();
    if (!client) {
      return;
    }

    if (client->IsPaused()) {
      client->AwakePauseLockWithMessage(message);
    } else {
      client->DispatchProtocolMessage(message);
    }
  }

  void disconnect() override {
    auto client = weakClient_.lock();
    if (!client) {
      return;
    }
    client->DisconnectFromReactFrontend();
  }

 private:
  std::weak_ptr<InspectorClient> weakClient_;
};

int InspectorClient::nextContextGroupId_ = 1;

InspectorClient::InspectorClient(
    std::shared_ptr<facebook::react::MessageQueueThread> jsQueue,
    v8::Local<v8::Context> context,
    const std::string &appName,
    const std::string &deviceName) {
  jsQueue_ = jsQueue;    
  isolate_ = context->GetIsolate();
  v8::HandleScope scopedHandle(isolate_);
  channel_.reset(new InspectorFrontend(this, context));
  inspector_ = v8_inspector::V8Inspector::create(isolate_, this);
  inspectorName_ = CreateInspectorName(appName, deviceName);
  v8_inspector::StringView inspectorNameStringView =
      ToStringView(inspectorName_);
  int contextGroupId = nextContextGroupId_++;
  session_ = inspector_->connect(
      contextGroupId, channel_.get(), inspectorNameStringView);
  context_.Reset(isolate_, context);

  inspector_->contextCreated(v8_inspector::V8ContextInfo(
      context, contextGroupId, inspectorNameStringView));
}

InspectorClient::~InspectorClient() {
  v8::HandleScope scopedHandle(isolate_);
  inspector_->contextDestroyed(context_.Get(isolate_));
  Disconnect();
}

void InspectorClient::runMessageLoopOnPause(int contextGroupId) {
  paused_ = true;
  while (paused_) {
    std::unique_lock<std::mutex> lock(pauseMutex_);
    pauseWaitable_.wait(lock);

    DispatchProtocolMessages(protocolMessageQueue_);
    protocolMessageQueue_.clear();
  }
}

void InspectorClient::quitMessageLoopOnPause() {
  paused_ = false;
  pauseWaitable_.notify_all();
}

v8::Local<v8::Context> InspectorClient::ensureDefaultContextInGroup(
    int group_id) {
  return context_.Get(isolate_);
}

void InspectorClient::ConnectToReactFrontend() {
  std::lock_guard<std::mutex> lock(connectionMutex_);

  auto weakClient = CreateWeakPtr();
  pageId_ = react::getInspectorInstance().addPage(
      inspectorName_,
      "V8",
      [weakClient = std::move(weakClient)](
          std::unique_ptr<react::IRemoteConnection> remoteConn) {
        auto client = weakClient.lock();
        if (client) {
          client->remoteConn_ = std::move(remoteConn);
        }
        return std::make_unique<LocalConnection>(weakClient);
      });
}

void InspectorClient::Disconnect() {
  DisconnectFromReactFrontend();
  std::lock_guard<std::mutex> lock(connectionMutex_);
  if (remoteConn_) {
    remoteConn_->onDisconnect();
  }
  react::getInspectorInstance().removePage(pageId_);
}

void InspectorClient::DisconnectFromReactFrontend() {
  DispatchProtocolMessages(
      {folly::toJson(
           folly::dynamic::object("method", "Debugger.disable")("id", 1e9)),
       folly::toJson(
           folly::dynamic::object("method", "Runtime.disable")("id", 1e9))});
}

void InspectorClient::SendRemoteMessage(
    const v8_inspector::StringView &message) {
  if (remoteConn_) {
    v8::Isolate *isolate = GetIsolate();
    v8::Locker locker(isolate);
    v8::Isolate::Scope scopedIsolate(isolate);
    v8::HandleScope scopedHandle(isolate);
    v8::Context::Scope scopedContext(GetContext().Get(isolate));

    std::string messageString = ToSTLString(isolate_, message);
    remoteConn_->onMessage(messageString);
  }
}

bool InspectorClient::IsPaused() {
  return paused_;
}

void InspectorClient::AwakePauseLockWithMessage(const std::string &message) {
  std::lock_guard<std::mutex> lock(pauseMutex_);
  protocolMessageQueue_.push_back(message);
  pauseWaitable_.notify_all();
}

void InspectorClient::DispatchProxy(const std::string &message) {
  std::string normalizedString = stripMetroCachePrevention(message);

  auto messageObj = folly::parseJson(message);
  auto method = messageObj["method"].asString();

  // For `v8::CpuProfiler` or some other modules with thread local storage, we should dispatch messages in the js thread.
  if (method == "Profiler.start" || method == "Profiler.stop") {
    jsQueue_->runOnQueue([this, normalizedString]() {
        v8::Isolate *isolate = GetIsolate();
        v8::Locker locker(isolate);
        v8::Isolate::Scope scopedIsolate(isolate);
        v8::HandleScope scopedHandle(isolate);
        v8::Context::Scope scopedContext(GetContext().Get(isolate));
        session_->dispatchProtocolMessage(ToStringView(normalizedString));
    });
    return;
  }

  session_->dispatchProtocolMessage(ToStringView(normalizedString));
}

void InspectorClient::DispatchProtocolMessage(const std::string &message) {
  auto session = GetInspectorSession();
  if (!session) {
    return;
  }
  v8::Isolate *isolate = GetIsolate();
  v8::Locker locker(isolate);
  v8::Isolate::Scope scopedIsolate(isolate);
  v8::HandleScope scopedHandle(isolate);
  v8::Context::Scope scopedContext(GetContext().Get(isolate));
  DispatchProxy(message);
}

void InspectorClient::DispatchProtocolMessages(
    const std::vector<std::string> &messages) {
  auto session = GetInspectorSession();
  if (!session) {
    return;
  }
  v8::Isolate *isolate = GetIsolate();
  v8::Locker locker(isolate);
  v8::Isolate::Scope scopedIsolate(isolate);
  v8::HandleScope scopedHandle(isolate);
  v8::Context::Scope scopedContext(GetContext().Get(isolate));

  for (auto &message : messages) {
    DispatchProxy(message);
  }
}

v8::Isolate *InspectorClient::GetIsolate() {
  return isolate_;
}

v8::Global<v8::Context> &InspectorClient::GetContext() {
  return context_;
}

v8_inspector::V8InspectorSession *InspectorClient::GetInspectorSession() {
  return session_ ? session_.get() : nullptr;
}

std::weak_ptr<InspectorClient> InspectorClient::CreateWeakPtr() {
  return std::weak_ptr<InspectorClient>(shared_from_this());
}

// static
std::string InspectorClient::CreateInspectorName(
    const std::string &appName,
    const std::string &deviceName) {
  std::ostringstream ss;
  ss << kInspectorName << " - " << appName;
  return ss.str();
}

} // namespace rnv8
