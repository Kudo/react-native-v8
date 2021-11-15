/**
 * Copyright (c) Facebook, Inc. and its affiliates.
 *
 * This source code is licensed under the MIT license found in the LICENSE
 * file in the root directory of this source tree.
 */

#include "V8ExecutorFactory.h"

#include <thread>

#include "cxxreact/MessageQueueThread.h"
#include "cxxreact/SystraceSection.h"
#include "jsi/v8runtime/V8RuntimeFactory.h"

using namespace facebook::jsi;

namespace facebook {
namespace react {

namespace {

std::unique_ptr<Runtime> makeV8RuntimeSystraced(const V8RuntimeConfig &config) {
  SystraceSection s("V8ExecutorFactory::makeV8RuntimeSystraced");
  return createV8Runtime(config);
}

} // namespace

std::unique_ptr<JSExecutor> V8ExecutorFactory::createJSExecutor(
    std::shared_ptr<ExecutorDelegate> delegate,
    std::shared_ptr<MessageQueueThread> jsQueue) {
  std::unique_ptr<Runtime> v8Runtime = makeV8RuntimeSystraced(config_);

  // Add js engine information to Error.prototype so in error reporting we
  // can send this information.
  auto errorPrototype = v8Runtime->global()
                            .getPropertyAsObject(*v8Runtime, "Error")
                            .getPropertyAsObject(*v8Runtime, "prototype");
  errorPrototype.setProperty(*v8Runtime, "jsEngine", "v8");

  return std::make_unique<V8Executor>(
      std::move(v8Runtime),
      delegate,
      jsQueue,
      timeoutInvoker_,
      runtimeInstaller_);
}

V8Executor::V8Executor(
    std::shared_ptr<jsi::Runtime> runtime,
    std::shared_ptr<ExecutorDelegate> delegate,
    std::shared_ptr<MessageQueueThread> jsQueue,
    const JSIScopedTimeoutInvoker &timeoutInvoker,
    RuntimeInstaller runtimeInstaller)
    : JSIExecutor(runtime, delegate, timeoutInvoker, runtimeInstaller) {}

} // namespace react
} // namespace facebook
