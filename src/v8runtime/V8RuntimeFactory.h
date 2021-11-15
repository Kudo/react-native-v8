#pragma once

#include <memory.h>
#include "jsi/jsi.h"
#include "jsi/v8runtime/V8RuntimeConfig.h"

namespace facebook {

std::unique_ptr<jsi::Runtime> createV8Runtime(const V8RuntimeConfig &config);

} // namespace facebook
