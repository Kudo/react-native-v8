/*
 * Copyright (c) Kudo Chien.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <jsireact/JSIExecutor.h>

namespace rnv8 {

class V8ExecutorFactory : public facebook::react::JSExecutorFactory {
 public:
  explicit V8ExecutorFactory(
      facebook::react::JSIExecutor::RuntimeInstaller runtimeInstaller)
      : runtimeInstaller_(std::move(runtimeInstaller)) {}

  std::unique_ptr<facebook::react::JSExecutor> createJSExecutor(
      std::shared_ptr<facebook::react::ExecutorDelegate> delegate,
      std::shared_ptr<facebook::react::MessageQueueThread> jsQueue) override;

 private:
  facebook::react::JSIExecutor::RuntimeInstaller runtimeInstaller_;
};

} // namespace rnv8
