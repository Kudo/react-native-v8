/*
 * Copyright (c) Kudo Chien.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <cxxreact/MessageQueueThread.h>
#include "V8RuntimeConfig.h"
#include "jsi/jsi.h"
#include "libplatform/libplatform.h"
#include "v8.h"

namespace rnv8 {

class V8Runtime;
class V8PointerValue;
class InspectorClient;

class V8Runtime : public facebook::jsi::Runtime {
 public:
  V8Runtime(
      std::unique_ptr<V8RuntimeConfig> config,
      std::shared_ptr<facebook::react::MessageQueueThread> jsQueue);
  V8Runtime(
      const V8Runtime *v8Runtime,
      std::unique_ptr<V8RuntimeConfig> config);
  ~V8Runtime();

  // Calling this function when the platform main runloop is idle
  void OnMainLoopIdle();

 private:
  v8::Local<v8::Context> CreateGlobalContext(v8::Isolate *isolate);
  facebook::jsi::Value ExecuteScript(
      v8::Isolate *isolate,
      const v8::Local<v8::String> &script,
      const std::string &sourceURL);
  void ReportException(v8::Isolate *isolate, v8::TryCatch *tryCatch) const;

  std::unique_ptr<v8::ScriptCompiler::CachedData> LoadCodeCacheIfNeeded(
      const std::string &sourceURL);
  bool SaveCodeCacheIfNeeded(
      const v8::Local<v8::Script> &script,
      const std::string &sourceURL,
      v8::ScriptCompiler::CachedData *cachedData);
  std::unique_ptr<v8::ScriptCompiler::Source> UseFakeSourceIfNeeded(
      const v8::ScriptOrigin &origin,
      v8::ScriptCompiler::CachedData *cachedData);

  enum InternalFieldType {
    kInvalid = 0,
    kHostObject = 1,
    kNativeState = 2,
    kMaxValue = kNativeState,
  };
  InternalFieldType GetInternalFieldType(v8::Local<v8::Object> object) const;

  static v8::Platform *GetPlatform();

  //
  // facebook::jsi::Runtime implementations
  //
 public:
  facebook::jsi::Value evaluateJavaScript(
      const std::shared_ptr<const facebook::jsi::Buffer> &buffer,
      const std::string &sourceURL) override;

  std::shared_ptr<const facebook::jsi::PreparedJavaScript> prepareJavaScript(
      const std::shared_ptr<const facebook::jsi::Buffer> &buffer,
      std::string sourceURL) override;
  facebook::jsi::Value evaluatePreparedJavaScript(
      const std::shared_ptr<const facebook::jsi::PreparedJavaScript> &js)
      override;

  bool drainMicrotasks(int maxMicrotasksHint = -1) override;

  facebook::jsi::Object global() override;
  std::string description() override;
  bool isInspectable() override;

 protected:
  PointerValue *cloneSymbol(const Runtime::PointerValue *pv) override;
#if REACT_NATIVE_TARGET_VERSION >= 70
  PointerValue *cloneBigInt(const Runtime::PointerValue *pv) override;
#endif
  PointerValue *cloneString(const Runtime::PointerValue *pv) override;
  PointerValue *cloneObject(const Runtime::PointerValue *pv) override;
  PointerValue *clonePropNameID(const Runtime::PointerValue *pv) override;

  facebook::jsi::PropNameID createPropNameIDFromAscii(
      const char *str,
      size_t length) override;
  facebook::jsi::PropNameID createPropNameIDFromUtf8(
      const uint8_t *utf8,
      size_t length) override;
  facebook::jsi::PropNameID createPropNameIDFromString(
      const facebook::jsi::String &str) override;
#if REACT_NATIVE_TARGET_VERSION >= 69
  facebook::jsi::PropNameID createPropNameIDFromSymbol(
      const facebook::jsi::Symbol &sym) override;
#endif
  std::string utf8(const facebook::jsi::PropNameID &) override;
  bool compare(
      const facebook::jsi::PropNameID &,
      const facebook::jsi::PropNameID &) override;

  std::string symbolToString(const facebook::jsi::Symbol &) override;

  facebook::jsi::BigInt createBigIntFromInt64(int64_t) override;
  facebook::jsi::BigInt createBigIntFromUint64(uint64_t) override;
  bool bigintIsInt64(const facebook::jsi::BigInt &) override;
  bool bigintIsUint64(const facebook::jsi::BigInt &) override;
  uint64_t truncate(const facebook::jsi::BigInt &) override;
  facebook::jsi::String bigintToString(const facebook::jsi::BigInt &, int)
      override;

  facebook::jsi::String createStringFromAscii(const char *str, size_t length)
      override;
  facebook::jsi::String createStringFromUtf8(const uint8_t *utf8, size_t length)
      override;
  std::string utf8(const facebook::jsi::String &) override;

  facebook::jsi::Object createObject() override;
  facebook::jsi::Object createObject(
      std::shared_ptr<facebook::jsi::HostObject> hostObject) override;
  std::shared_ptr<facebook::jsi::HostObject> getHostObject(
      const facebook::jsi::Object &) override;
  facebook::jsi::HostFunctionType &getHostFunction(
      const facebook::jsi::Function &) override;

  bool hasNativeState(const facebook::jsi::Object &) override;
  std::shared_ptr<facebook::jsi::NativeState> getNativeState(
      const facebook::jsi::Object &) override;
  void setNativeState(
      const facebook::jsi::Object &,
      std::shared_ptr<facebook::jsi::NativeState> state) override;

  facebook::jsi::Value getProperty(
      const facebook::jsi::Object &,
      const facebook::jsi::PropNameID &name) override;
  facebook::jsi::Value getProperty(
      const facebook::jsi::Object &,
      const facebook::jsi::String &name) override;
  bool hasProperty(
      const facebook::jsi::Object &,
      const facebook::jsi::PropNameID &name) override;
  bool hasProperty(
      const facebook::jsi::Object &,
      const facebook::jsi::String &name) override;
  void setPropertyValue(
      facebook::jsi::Object &,
      const facebook::jsi::PropNameID &name,
      const facebook::jsi::Value &value) override;
  void setPropertyValue(
      facebook::jsi::Object &,
      const facebook::jsi::String &name,
      const facebook::jsi::Value &value) override;

  bool isArray(const facebook::jsi::Object &) const override;
  bool isArrayBuffer(const facebook::jsi::Object &) const override;
  bool isFunction(const facebook::jsi::Object &) const override;
  bool isHostObject(const facebook::jsi::Object &) const override;
  bool isHostFunction(const facebook::jsi::Function &) const override;
  facebook::jsi::Array getPropertyNames(const facebook::jsi::Object &) override;

  facebook::jsi::WeakObject createWeakObject(
      const facebook::jsi::Object &) override;
  facebook::jsi::Value lockWeakObject(facebook::jsi::WeakObject &) override;

  facebook::jsi::Array createArray(size_t length) override;
  facebook::jsi::ArrayBuffer createArrayBuffer(
      std::shared_ptr<facebook::jsi::MutableBuffer> buffer) override;
  size_t size(const facebook::jsi::Array &) override;
  size_t size(const facebook::jsi::ArrayBuffer &) override;
  uint8_t *data(const facebook::jsi::ArrayBuffer &) override;
  facebook::jsi::Value getValueAtIndex(const facebook::jsi::Array &, size_t i)
      override;
  void setValueAtIndexImpl(
      facebook::jsi::Array &,
      size_t i,
      const facebook::jsi::Value &value) override;

  facebook::jsi::Function createFunctionFromHostFunction(
      const facebook::jsi::PropNameID &name,
      unsigned int paramCount,
      facebook::jsi::HostFunctionType func) override;
  facebook::jsi::Value call(
      const facebook::jsi::Function &,
      const facebook::jsi::Value &jsThis,
      const facebook::jsi::Value *args,
      size_t count) override;
  facebook::jsi::Value callAsConstructor(
      const facebook::jsi::Function &,
      const facebook::jsi::Value *args,
      size_t count) override;

  bool strictEquals(
      const facebook::jsi::Symbol &a,
      const facebook::jsi::Symbol &b) const override;
#if REACT_NATIVE_TARGET_VERSION >= 70
  bool strictEquals(
      const facebook::jsi::BigInt &a,
      const facebook::jsi::BigInt &b) const override;
#endif
  bool strictEquals(
      const facebook::jsi::String &a,
      const facebook::jsi::String &b) const override;
  bool strictEquals(
      const facebook::jsi::Object &a,
      const facebook::jsi::Object &b) const override;

  bool instanceOf(
      const facebook::jsi::Object &o,
      const facebook::jsi::Function &f) override;

 private:
  friend class V8PointerValue;
  friend class JSIV8ValueConverter;

  //
  // JS function/object handler callbacks
  //
 private:
  // For `global._v8runtime()`
  static void GetRuntimeInfo(const v8::FunctionCallbackInfo<v8::Value> &args);

  // For `HostFunctionContainer()`, will call underlying HostFunction
  static void OnHostFuncionContainerCallback(
      const v8::FunctionCallbackInfo<v8::Value> &args);

 private:
  static std::unique_ptr<v8::Platform> s_platform;

 private:
  std::unique_ptr<V8RuntimeConfig> config_;
  std::unique_ptr<v8::ArrayBuffer::Allocator> arrayBufferAllocator_;
  std::unique_ptr<v8::StartupData> snapshotBlob_;
  v8::Isolate *isolate_;
  v8::Global<v8::Context> context_;
  std::shared_ptr<InspectorClient> inspectorClient_;
  bool isSharedRuntime_ = false;
  std::shared_ptr<facebook::react::MessageQueueThread> jsQueue_;
};

} // namespace rnv8
