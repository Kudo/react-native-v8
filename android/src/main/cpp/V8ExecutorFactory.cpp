/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 * Copyright (c) Kudo Chien.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include "V8ExecutorFactory.h"

#include <thread>

#include "V8RuntimeFactory.h"
#include "cxxreact/MessageQueueThread.h"
#include "cxxreact/SystraceSection.h"

namespace jsi = facebook::jsi;
namespace react = facebook::react;

namespace rnv8 {

namespace {

std::unique_ptr<jsi::Runtime> makeV8RuntimeSystraced(
    std::unique_ptr<V8RuntimeConfig> config, 
    std::shared_ptr<react::MessageQueueThread> jsQueue) {
  react::SystraceSection s("V8ExecutorFactory::makeV8RuntimeSystraced");
  return createV8Runtime(std::move(config), jsQueue);
}

} // namespace

std::unique_ptr<react::JSExecutor> V8ExecutorFactory::createJSExecutor(
    std::shared_ptr<react::ExecutorDelegate> delegate,
    std::shared_ptr<react::MessageQueueThread> jsQueue) {
  std::unique_ptr<jsi::Runtime> v8Runtime =
      makeV8RuntimeSystraced(std::move(config_), jsQueue);

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
    std::shared_ptr<react::ExecutorDelegate> delegate,
    std::shared_ptr<react::MessageQueueThread> jsQueue,
    const react::JSIScopedTimeoutInvoker &timeoutInvoker,
    RuntimeInstaller runtimeInstaller)
    : JSIExecutor(runtime, delegate, timeoutInvoker, runtimeInstaller) {}

} // namespace rnv8
