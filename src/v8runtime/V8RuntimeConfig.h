/*
 * Copyright (c) Kudo Chien.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <cxxreact/JSBigString.h>
#include <string>
#include <vector>

namespace rnv8 {

struct V8RuntimeConfig {
  V8RuntimeConfig() {}
  ~V8RuntimeConfig() = default;

  V8RuntimeConfig(const V8RuntimeConfig &rhs) = delete;
  V8RuntimeConfig &operator=(const V8RuntimeConfig &rhs) = delete;

  V8RuntimeConfig(V8RuntimeConfig &&rhs) = default;
  V8RuntimeConfig &operator=(V8RuntimeConfig &&rhs) = default;

  // Olson timezone ID
  std::string timezoneId;

  // true to enable V8 inspector for Chrome DevTools
  bool enableInspector = false;

  // Application name
  std::string appName;

  // Device name
  std::string deviceName;

  // Startup snapshot blob
  std::unique_ptr<const facebook::react::JSBigString> snapshotBlob;

  // Bytecode caching mode
  //   0: disable bytecode cache
  //   1: enable bytecode cache
  //   2: use prebuilt bytecode cache
  //   3: enable bytecode cache + use stub bundle
  int codecacheMode;

  // codecache path
  std::string codecachePath;

  // Prebuild codecache blob
  std::unique_ptr<const facebook::react::JSBigString> prebuiltCodecacheBlob;
};

} // namespace rnv8
