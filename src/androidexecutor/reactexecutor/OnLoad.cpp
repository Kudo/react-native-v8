/**
 * Copyright (c) Facebook, Inc. and its affiliates.
 *
 * This source code is licensed under the MIT license found in the LICENSE
 * file in the root directory of this source tree.
 */
#include <V8ExecutorFactory.h>
#include <fbjni/fbjni.h>
#include <folly/Memory.h>
#include <jni.h>
#include <react/jni/JReactMarker.h>
#include <react/jni/JSLogging.h>
#include <react/jni/JavaScriptExecutorHolder.h>

#include "jsi/v8runtime/V8RuntimeConfig.h"

namespace facebook {
namespace react {

static void installBindings(jsi::Runtime &runtime) {
  react::Logger androidLogger =
      static_cast<void (*)(const std::string &, unsigned int)>(
          &reactAndroidLoggingHook);
  react::bindNativeLogger(runtime, androidLogger);
}

class V8ExecutorHolder
    : public jni::HybridClass<V8ExecutorHolder, JavaScriptExecutorHolder> {
 public:
  static constexpr auto kJavaDescriptor =
      "Lcom/facebook/v8/reactexecutor/V8Executor;";

  static jni::local_ref<jhybriddata> initHybrid(
      jni::alias_ref<jclass>,
      const std::string &timezoneId,
      bool enableInspector,
      const std::string &appName,
      const std::string &deviceName) {
    JReactMarker::setLogPerfMarkerIfNeeded();

    facebook::V8RuntimeConfig config;
    config.timezoneId = timezoneId;
    config.enableInspector = enableInspector;
    config.appName = appName;
    config.deviceName = deviceName;

    return makeCxxInstance(folly::make_unique<V8ExecutorFactory>(
        installBindings, JSIExecutor::defaultTimeoutInvoker, config));
  }

  static void registerNatives() {
    registerHybrid(
        {makeNativeMethod("initHybrid", V8ExecutorHolder::initHybrid)});
  }

 private:
  friend HybridBase;
  using HybridBase::HybridBase;
};

} // namespace react
} // namespace facebook

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved) {
  return facebook::jni::initialize(
      vm, [] { facebook::react::V8ExecutorHolder::registerNatives(); });
}
