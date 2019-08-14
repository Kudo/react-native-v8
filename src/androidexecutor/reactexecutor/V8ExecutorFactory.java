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

  @Override
  public JavaScriptExecutor create() {
    return new V8Executor();
  }

  @Override
  public String toString() {
    return "JSIExecutor+V8Runtime";
  }
}
