/**
 * Copyright (c) Facebook, Inc. and its affiliates.
 *
 * This source code is licensed under the MIT license found in the LICENSE
 * file in the root directory of this source tree.
 */
package com.facebook.v8.reactexecutor;

import android.os.Build;
import android.os.StrictMode;
import java.util.TimeZone;

public final class V8RuntimeConfig {
  public String timezoneId;
  public boolean enableInspector;
  public String appName;
  public String deviceName;

  public static V8RuntimeConfig createDefault() {
    final V8RuntimeConfig config = new V8RuntimeConfig();
    config.timezoneId = getTimezoneId();
    config.enableInspector = false;
    return config;
  }

  private static String getTimezoneId() {
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
