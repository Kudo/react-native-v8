/**
 * Copyright (c) Facebook, Inc. and its affiliates.
 *
 * This source code is licensed under the MIT license found in the LICENSE
 * file in the root directory of this source tree.
 */
package com.facebook.v8.reactexecutor;

import android.os.Build;
import android.os.StrictMode;
import com.facebook.react.bridge.JavaScriptExecutor;
import com.facebook.react.bridge.JavaScriptExecutorFactory;
import java.util.TimeZone;

public class V8ExecutorFactory implements JavaScriptExecutorFactory {
  private static final String TAG = "V8";

  @Override
  public JavaScriptExecutor create() {
    return new V8Executor(getTimezoneId());
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

  private String getTimezoneId() {
    if (Build.VERSION.SDK_INT <= Build.VERSION_CODES.N) {
      final StrictMode.ThreadPolicy oldPolicy =
          StrictMode.allowThreadDiskReads();
      final String timezoneId = TimeZone.getDefault().getID();
      StrictMode.setThreadPolicy(oldPolicy);
      return timezoneId;
    } else {
      final String timezoneId = TimeZone.getDefault().getID();
      return timezoneId;
    }
  }
}
