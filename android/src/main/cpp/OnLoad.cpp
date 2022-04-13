/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 * Copyright (c) Kudo Chien.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <cxxreact/JSBigString.h>
#include <fbjni/fbjni.h>
#include <folly/Memory.h>
#include <glog/logging.h>
#include <jni.h>
#include <react/jni/JReactMarker.h>
#include <react/jni/JSLoader.h>
#include <react/jni/JSLogging.h>
#include <react/jni/JavaScriptExecutorHolder.h>

#include "V8ExecutorFactory.h"
#include "V8RuntimeConfig.h"

namespace jni = facebook::jni;
namespace jsi = facebook::jsi;
namespace react = facebook::react;

namespace rnv8 {

static void installBindings(jsi::Runtime &runtime) {
  react::Logger androidLogger =
      static_cast<void (*)(const std::string &, unsigned int)>(
          &react::reactAndroidLoggingHook);
  react::bindNativeLogger(runtime, androidLogger);
}

static std::vector<char> loadBlob(
    jni::alias_ref<react::JAssetManager::javaobject> assetManager,
    const std::string &blobPath) {
  std::vector<char> result;
  std::string assetScheme = "assets://";
  if (blobPath.substr(0, assetScheme.size()) == assetScheme) {
    std::string assetName = blobPath.substr(assetScheme.size());
    auto manager = react::extractAssetManager(assetManager);
    try {
      auto script = react::loadScriptFromAssets(manager, assetName);
      result.resize(script->size());
      std::memcpy(result.data(), script->c_str(), script->size());
    } catch (...) {
      LOG(ERROR) << "Unable to loadBlob from AssetManager - name[" << assetName
                 << "]";
    }
  } else {
    auto script = react::JSBigFileString::fromPath(blobPath);
    result.resize(script->size());
    std::memcpy(result.data(), script->c_str(), script->size());
  }

  return result;
}

class V8ExecutorHolder
    : public jni::
          HybridClass<V8ExecutorHolder, react::JavaScriptExecutorHolder> {
 public:
  static constexpr auto kJavaDescriptor =
      "Lio/csie/kudo/reactnative/v8/executor/V8Executor;";

  static jni::local_ref<jhybriddata> initHybrid(
      jni::alias_ref<jclass>,
      jni::alias_ref<react::JAssetManager::javaobject> assetManager,
      const std::string &timezoneId,
      bool enableInspector,
      const std::string &appName,
      const std::string &deviceName,
      const std::string &snapshotBlobPath) {
    react::JReactMarker::setLogPerfMarkerIfNeeded();

    V8RuntimeConfig config;
    config.timezoneId = timezoneId;
    config.enableInspector = enableInspector;
    config.appName = appName;
    config.deviceName = deviceName;
    if (!snapshotBlobPath.empty()) {
      config.snapshotBlob = loadBlob(assetManager, snapshotBlobPath);
    }

    return makeCxxInstance(folly::make_unique<V8ExecutorFactory>(
        installBindings, react::JSIExecutor::defaultTimeoutInvoker, config));
  }

  static void registerNatives() {
    registerHybrid(
        {makeNativeMethod("initHybrid", V8ExecutorHolder::initHybrid)});
  }

 private:
  friend HybridBase;
  using HybridBase::HybridBase;
};

} // namespace rnv8

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved) {
  return facebook::jni::initialize(
      vm, [] { rnv8::V8ExecutorHolder::registerNatives(); });
}
