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
import androidx.annotation.IntDef;
import androidx.annotation.Nullable;
import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.util.TimeZone;

public final class V8RuntimeConfig {
  // Olson timezone ID
  public String timezoneId;

  // true to enable V8 inspector for Chrome DevTools
  public boolean enableInspector;

  // Application name
  public String appName;

  // Device name
  public String deviceName;

  // Startup snapshot blob
  @Nullable public String snapshotBlobPath;

  @IntDef(
      {CODECACHE_MODE_NONE,
       CODECACHE_MODE_NORMAL,
       CODECACHE_MODE_PREBUILT,
       CODECACHE_MODE_NORMAL_WITH_STUB_BUNDLE})
  @Retention(RetentionPolicy.SOURCE)
  private @interface CodecacheMode {}

  // Disable bytecode caching
  public static final int CODECACHE_MODE_NONE = 0;
  // Classic v8 bytecode caching
  public static final int CODECACHE_MODE_NORMAL = 1;
  // **EXPERIMENTAL** Prebuilt bytecode caching
  public static final int CODECACHE_MODE_PREBUILT = 2;
  // **EXPERIMENTAL** Classic v8 bytecode caching + loading stub JS bundle when
  // cache existed
  public static final int CODECACHE_MODE_NORMAL_WITH_STUB_BUNDLE = 3;

  // Bytecode caching mode
  public @CodecacheMode int codecacheMode;

  // Codecache blob
  @Nullable public String codecachePath;

  public static V8RuntimeConfig createDefault() {
    final V8RuntimeConfig config = new V8RuntimeConfig();
    config.timezoneId = getTimezoneId();
    config.enableInspector = false;
    config.snapshotBlobPath = null;
    config.codecacheMode = CODECACHE_MODE_NONE;
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
