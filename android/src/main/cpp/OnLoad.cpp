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

static std::unique_ptr<const react::JSBigString> loadBlob(
    jni::alias_ref<react::JAssetManager::javaobject> assetManager,
    const std::string &blobPath) {
  std::string assetScheme = "assets://";
  if (blobPath.substr(0, assetScheme.size()) == assetScheme) {
    std::string assetName = blobPath.substr(assetScheme.size());
    auto manager = react::extractAssetManager(assetManager);
    try {
      return react::loadScriptFromAssets(manager, assetName);
    } catch (...) {
      LOG(ERROR) << "Unable to loadBlob from AssetManager - name[" << assetName
                 << "]";
    }
  } else {
    return react::JSBigFileString::fromPath(blobPath);
  }

  return nullptr;
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
      const std::string &snapshotBlobPath,
      int codecacheMode,
      const std::string &codecachePath) {
    react::JReactMarker::setLogPerfMarkerIfNeeded();

    auto config = std::make_unique<V8RuntimeConfig>();
    config->timezoneId = timezoneId;
    config->enableInspector = enableInspector;
    config->appName = appName;
    config->deviceName = deviceName;
    if (!snapshotBlobPath.empty()) {
      config->snapshotBlob =
          std::move(loadBlob(assetManager, snapshotBlobPath));
    }
    config->codecacheMode =
        static_cast<V8RuntimeConfig::CodecacheMode>(codecacheMode);
    config->codecachePath = codecachePath;
    if (config->codecacheMode == V8RuntimeConfig::CodecacheMode::kPrebuilt &&
        !codecachePath.empty()) {
      config->prebuiltCodecacheBlob =
          std::move(loadBlob(assetManager, codecachePath));
    }

    return makeCxxInstance(folly::make_unique<V8ExecutorFactory>(
        installBindings,
        react::JSIExecutor::defaultTimeoutInvoker,
        std::move(config)));
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
