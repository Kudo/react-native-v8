/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 * Copyright (c) Kudo Chien.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */
package io.csie.kudo.reactnative.v8.executor;

import android.content.res.AssetManager;
import android.os.Build;
import com.facebook.jni.HybridData;
import com.facebook.react.bridge.JavaScriptExecutor;
import com.facebook.soloader.SoLoader;
import io.csie.kudo.reactnative.v8.BuildConfig;

public class V8Executor extends JavaScriptExecutor {
  static {
    SoLoader.loadLibrary("v8executor");
  }

  V8Executor(final AssetManager assetManager, final V8RuntimeConfig config) {
    super(initHybrid(
        assetManager,
        config.timezoneId,
        config.enableInspector,
        config.appName,
        config.deviceName,
        config.snapshotBlobPath != null ? config.snapshotBlobPath
                                        : loadDefaultSnapshotBlobPath(),
        config.codecacheMode,
        config.codecachePath != null
            ? config.codecachePath
            : loadDefaultCodecachePath(config.codecacheMode)));
  }

  @Override
  public String getName() {
    return "V8Executor";
  }

  private static String loadDefaultSnapshotBlobPath() {
    if (BuildConfig.V8_USE_SNAPSHOT) {
      return "assets://" + Build.SUPPORTED_ABIS[0] + "/snapshot_blob.bin";
    }
    return "";
  }

  private static String loadDefaultCodecachePath(int codecacheMode) {
    if (codecacheMode == 2 && BuildConfig.V8_USE_PREBUILT_CACHE) {
      return "assets://" + Build.SUPPORTED_ABIS[0] + "/v8codecache.bin";
    }
    return "";
  }

  private static native HybridData initHybrid(
      AssetManager assetManager,
      String timezoneId,
      boolean enableInspector,
      String appName,
      String deviceName,
      String snapshotBlobPath,
      int codecacheMode,
      String codecachePath);
}
