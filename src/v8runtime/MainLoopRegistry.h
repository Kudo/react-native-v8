/*
 * Copyright (c) Kudo Chien.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <algorithm>
#include <functional>
#include <mutex>
#include <vector>

namespace rnv8 {

class MainLoopRegistry {
 public:
  using Callback = std::function<void()>;

  static MainLoopRegistry *GetInstance();

 public:
  void RegisterCallback(Callback callback);

  void UnregisterCallback(const Callback &callback);

  void OnMainLoopIdle();

 private:
  MainLoopRegistry() = default;
  ~MainLoopRegistry() = default;

  MainLoopRegistry(const MainLoopRegistry &) = delete;
  MainLoopRegistry &operator=(const MainLoopRegistry &) = delete;

  std::vector<Callback> callbacks_;
  std::mutex mutex_;
};

} // namespace rnv8
