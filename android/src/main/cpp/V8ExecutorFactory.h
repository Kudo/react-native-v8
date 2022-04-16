/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 * Copyright (c) Kudo Chien.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <jsireact/JSIExecutor.h>
#include "V8RuntimeConfig.h"

namespace rnv8 {

class V8ExecutorFactory : public facebook::react::JSExecutorFactory {
 public:
  explicit V8ExecutorFactory(
      facebook::react::JSIExecutor::RuntimeInstaller runtimeInstaller,
      const facebook::react::JSIScopedTimeoutInvoker &timeoutInvoker,
      std::unique_ptr<V8RuntimeConfig> config)
      : runtimeInstaller_(runtimeInstaller),
        timeoutInvoker_(timeoutInvoker),
        config_(std::move(config)) {
    assert(timeoutInvoker_ && "Should not have empty timeoutInvoker");
  }

  std::unique_ptr<facebook::react::JSExecutor> createJSExecutor(
      std::shared_ptr<facebook::react::ExecutorDelegate> delegate,
      std::shared_ptr<facebook::react::MessageQueueThread> jsQueue) override;

 private:
  facebook::react::JSIExecutor::RuntimeInstaller runtimeInstaller_;
  facebook::react::JSIScopedTimeoutInvoker timeoutInvoker_;
  std::unique_ptr<V8RuntimeConfig> config_;
};

class V8Executor : public facebook::react::JSIExecutor {
 public:
  V8Executor(
      std::shared_ptr<facebook::jsi::Runtime> runtime,
      std::shared_ptr<facebook::react::ExecutorDelegate> delegate,
      std::shared_ptr<facebook::react::MessageQueueThread> jsQueue,
      const facebook::react::JSIScopedTimeoutInvoker &timeoutInvoker,
      RuntimeInstaller runtimeInstaller);

 private:
  facebook::react::JSIScopedTimeoutInvoker timeoutInvoker_;
};

} // namespace rnv8
