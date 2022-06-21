/*
 * Copyright (c) Kudo Chien.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include "V8RuntimeFactory.h"

#include "V8Runtime.h"

namespace rnv8 {

std::unique_ptr<facebook::jsi::Runtime> createV8Runtime(
    std::unique_ptr<V8RuntimeConfig> config,
    std::shared_ptr<facebook::react::MessageQueueThread> jsQueue) {
  return std::make_unique<V8Runtime>(std::move(config), jsQueue);
}

std::unique_ptr<facebook::jsi::Runtime> createSharedV8Runtime(
    const facebook::jsi::Runtime *sharedRuntime,
    std::unique_ptr<V8RuntimeConfig> config) {
  auto *sharedV8Runtime = dynamic_cast<const V8Runtime *>(sharedRuntime);
  return std::make_unique<V8Runtime>(sharedV8Runtime, std::move(config));
}

} // namespace rnv8
