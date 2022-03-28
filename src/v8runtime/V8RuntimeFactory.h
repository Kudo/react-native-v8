#pragma once

#include <memory.h>
#include "V8RuntimeConfig.h"
#include "jsi/jsi.h"

namespace facebook {

std::unique_ptr<jsi::Runtime> createV8Runtime(const V8RuntimeConfig &config);

} // namespace facebook
