/*
 * Copyright (c) Kudo Chien.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include "MainLoopRegistry.h"

namespace rnv8 {

// static
MainLoopRegistry *MainLoopRegistry::GetInstance() {
  static MainLoopRegistry instance;
  return &instance;
}

void MainLoopRegistry::RegisterCallback(Callback callback) {
  std::lock_guard<std::mutex> lock(mutex_);
  callbacks_.push_back(std::move(callback));
}

void MainLoopRegistry::UnregisterCallback(const Callback &callback) {
  std::lock_guard<std::mutex> lock(mutex_);
  callbacks_.erase(
      std::remove_if(
          callbacks_.begin(),
          callbacks_.end(),
          [&callback](const Callback &registered_callback) {
            return callback.target_type() ==
                registered_callback.target_type() &&
                callback.target<void()>() ==
                registered_callback.target<void()>();
          }),
      callbacks_.end());
}

void MainLoopRegistry::OnMainLoopIdle() {
  std::lock_guard<std::mutex> lock(mutex_);
  for (const auto &callback : callbacks_) {
    callback();
  }
}

} // namespace rnv8
