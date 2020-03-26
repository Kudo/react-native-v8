/**
 * Copyright (c) Facebook, Inc. and its affiliates.
 *
 * This source code is licensed under the MIT license found in the LICENSE
 * file in the root directory of this source tree.
 */
package com.facebook.v8.reactexecutor;

import com.facebook.react.bridge.JavaScriptExecutor;
import com.facebook.react.bridge.JavaScriptExecutorFactory;

public class V8ExecutorFactory implements JavaScriptExecutorFactory {
  private static final String TAG = "V8";

  private final V8RuntimeConfig mConfig;

  public V8ExecutorFactory() {
    this(V8RuntimeConfig.createDefault());
  }

  public V8ExecutorFactory(final V8RuntimeConfig config) {
    mConfig = config;
  }

  @Override
  public JavaScriptExecutor create() {
    return new V8Executor(mConfig);
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
}
