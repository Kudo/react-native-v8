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
import androidx.annotation.Nullable;
import com.facebook.react.bridge.JavaScriptExecutor;
import com.facebook.react.bridge.JavaScriptExecutorFactory;
import io.csie.kudo.reactnative.v8.BuildConfig;
import java.io.File;

public class V8ExecutorFactory implements JavaScriptExecutorFactory {
  private static final String TAG = "V8";

  private final V8RuntimeConfig mConfig;
  private final AssetManager mAssetManager;

  public V8ExecutorFactory(final Context context) {
    this(context, V8RuntimeConfig.createDefault());
  }

  public V8ExecutorFactory(
      final Context context,
      final String appName,
      final String deviceName,
      final boolean useDeveloperSupport) {
    mConfig = V8RuntimeConfig.createDefault();
    mConfig.appName = appName;
    mConfig.deviceName = deviceName;
    mConfig.enableInspector = useDeveloperSupport;
    mAssetManager = context.getAssets();
  }

  public V8ExecutorFactory(
      final Context context,
      final V8RuntimeConfig config) {
    mConfig = config;
    mAssetManager = context.getAssets();
  }

  @Override
  public JavaScriptExecutor create() {
    return new V8Executor(mAssetManager, mConfig);
  }

  @Override
  public void startSamplingProfiler() {
    throw new UnsupportedOperationException(
        "Starting sampling profiler not supported on " + toString());
  }

  @Override
  public void stopSamplingProfiler(String filename) {
    throw new UnsupportedOperationException(
        "Stopping sampling profiler not supported on " + toString());
  }

  @Override
  public String toString() {
    return "JSIExecutor+V8Runtime";
  }

  @Nullable
  public static String getBundleAssetName(
      final Context context,
      final boolean useDeveloperSupport) {
    if (useDeveloperSupport) {
      return null;
    }
    final boolean hasCache =
        new File(context.getCodeCacheDir(), "v8codecache.bin").exists();
    if (BuildConfig.V8_USE_CACHE_WITH_STUB_BUNDLE && hasCache) {
      return "stub.bundle";
    }
    return null;
  }
}
