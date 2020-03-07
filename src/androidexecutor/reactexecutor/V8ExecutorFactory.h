/**
 * Copyright (c) Facebook, Inc. and its affiliates.
 *
 * This source code is licensed under the MIT license found in the LICENSE
 * file in the root directory of this source tree.
 */

#pragma once

#include <jsireact/JSIExecutor.h>

namespace facebook {
namespace react {

class V8ExecutorFactory : public JSExecutorFactory {
 public:
  explicit V8ExecutorFactory(
      JSIExecutor::RuntimeInstaller runtimeInstaller,
      const JSIScopedTimeoutInvoker &timeoutInvoker,
      const std::string &timezoneId)
      : runtimeInstaller_(runtimeInstaller),
        timeoutInvoker_(timeoutInvoker),
        timezoneId_(timezoneId) {
    assert(timeoutInvoker_ && "Should not have empty timeoutInvoker");
  }

  std::unique_ptr<JSExecutor> createJSExecutor(
      std::shared_ptr<ExecutorDelegate> delegate,
      std::shared_ptr<MessageQueueThread> jsQueue) override;

 private:
  JSIExecutor::RuntimeInstaller runtimeInstaller_;
  JSIScopedTimeoutInvoker timeoutInvoker_;
  std::string timezoneId_;
};

class V8Executor : public JSIExecutor {
 public:
  V8Executor(
      std::shared_ptr<jsi::Runtime> runtime,
      std::shared_ptr<ExecutorDelegate> delegate,
      std::shared_ptr<MessageQueueThread> jsQueue,
      const JSIScopedTimeoutInvoker &timeoutInvoker,
      RuntimeInstaller runtimeInstaller);

 private:
  JSIScopedTimeoutInvoker timeoutInvoker_;
};

} // namespace react
} // namespace facebook
