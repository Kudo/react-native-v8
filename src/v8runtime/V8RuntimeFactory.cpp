#include "V8RuntimeFactory.h"

#include "V8Runtime.h"

namespace facebook {

std::unique_ptr<jsi::Runtime> createV8Runtime(const std::string &timezoneId) {
  return std::make_unique<V8Runtime>(timezoneId);
}

} // namespace facebook
