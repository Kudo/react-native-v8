/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 * Copyright (c) Kudo Chien.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */
package io.csie.kudo.reactnative.v8.executor;

import android.content.Context;
import android.content.res.AssetManager;
import android.os.Build;
import com.facebook.jni.HybridData;
import com.facebook.react.bridge.JavaScriptExecutor;
import com.facebook.soloader.SoLoader;
import io.csie.kudo.reactnative.v8.BuildConfig;
import java.io.File;

public class V8Executor extends JavaScriptExecutor {
  static {
    SoLoader.loadLibrary("v8executor");
  }

  V8Executor(final Context context, final V8RuntimeConfig config) {
    super(initHybrid(
        context.getAssets(),
        config.timezoneId,
        config.enableInspector,
        config.appName,
        config.deviceName,
        config.snapshotBlobPath != null ? config.snapshotBlobPath
                                        : loadDefaultSnapshotBlobPath(),
        config.codecacheMode,
        config.codecachePath != null
            ? config.codecachePath
            : loadDefaultCodecachePath(context, config.codecacheMode)));
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

  private static String loadDefaultCodecachePath(
      final Context context,
      int codecacheMode) {
    if (codecacheMode == V8RuntimeConfig.CODECACHE_MODE_PREBUILT) {
      return "assets://" + Build.SUPPORTED_ABIS[0] + "/v8codecache.bin";
    }
    return new File(context.getCodeCacheDir(), "v8codecache.bin").toString();
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
