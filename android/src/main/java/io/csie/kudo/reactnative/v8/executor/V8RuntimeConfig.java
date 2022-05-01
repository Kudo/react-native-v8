/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 * Copyright (c) Kudo Chien.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */
package io.csie.kudo.reactnative.v8.executor;

import android.os.Build;
import android.os.StrictMode;
import androidx.annotation.Nullable;
import java.util.TimeZone;

public final class V8RuntimeConfig {
  // Olson timezone ID
  public String timezoneId;

  // true to enable V8 inspector for Chrome DevTools
  public boolean enableInspector;

  // Application name
  public String appName;

  // Device nam
  public String deviceName;

  // Startup snapshot blob
  @Nullable public String snapshotBlobPath;

  // Bytecode caching mode
  //   0: disable bytecode cache
  //   1: enable bytecode cache
  //   2: use prebuilt bytecode cache
  //   3: enable bytecode cache + use stub bundle
  public int codecacheMode;

  // Codecache blob
  @Nullable public String codecachePath;

  public static V8RuntimeConfig createDefault() {
    final V8RuntimeConfig config = new V8RuntimeConfig();
    config.timezoneId = getTimezoneId();
    config.enableInspector = false;
    config.snapshotBlobPath = null;
    config.codecacheMode = 0;
    config.codecachePath = null;
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
