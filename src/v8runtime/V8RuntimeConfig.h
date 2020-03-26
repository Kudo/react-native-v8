#pragma once

namespace facebook {

struct V8RuntimeConfig {
  V8RuntimeConfig() : enableInspector(false) {}
  V8RuntimeConfig(const V8RuntimeConfig &rhs) = default;
  ~V8RuntimeConfig() = default;

  // Olson timezone ID
  std::string timezoneId;

  // true to enable V8 inspector for Chrome DevTools
  bool enableInspector = false;
};

} // namespace facebook
