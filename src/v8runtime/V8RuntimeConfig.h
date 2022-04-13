/*
 * Copyright (c) Kudo Chien.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <string>
#include <vector>

namespace rnv8 {

struct V8RuntimeConfig {
  V8RuntimeConfig() {}
  V8RuntimeConfig(const V8RuntimeConfig &rhs) = default;
  ~V8RuntimeConfig() = default;

  // Olson timezone ID
  std::string timezoneId;

  // true to enable V8 inspector for Chrome DevTools
  bool enableInspector = false;

  // Application name
  std::string appName;

  // Device name
  std::string deviceName;

  // Startup snapshot blob
  std::vector<char> snapshotBlob;
};

} // namespace rnv8
