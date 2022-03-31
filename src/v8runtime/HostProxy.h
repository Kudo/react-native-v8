/*
 * Copyright (c) Kudo Chien.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include "V8Runtime.h"
#include "jsi/jsi.h"
#include "v8.h"

namespace rnv8 {

class HostObjectProxy {
 public:
  HostObjectProxy(
      V8Runtime &runtime,
      v8::Isolate *isolate,
      std::shared_ptr<facebook::jsi::HostObject> hostObject);

  void BindFinalizer(const v8::Local<v8::Object> &object);

  std::shared_ptr<facebook::jsi::HostObject> GetHostObject();

 public:
  static void Getter(
      v8::Local<v8::Name> property,
      const v8::PropertyCallbackInfo<v8::Value> &info);

  static void Setter(
      v8::Local<v8::Name> property,
      v8::Local<v8::Value> value,
      const v8::PropertyCallbackInfo<v8::Value> &info);

  static void Enumerator(const v8::PropertyCallbackInfo<v8::Array> &info);

  static void Finalizer(const v8::WeakCallbackInfo<HostObjectProxy> &data);

 private:
  V8Runtime &runtime_;
  v8::Isolate *isolate_;
  std::shared_ptr<facebook::jsi::HostObject> hostObject_;
  v8::Global<v8::Object> weakHandle_;
};

class HostFunctionProxy {
 public:
  HostFunctionProxy(
      V8Runtime &runtime,
      v8::Isolate *isolate,
      facebook::jsi::HostFunctionType &&hostFunction);

  void BindFinalizer(const v8::Local<v8::Object> &object);

  facebook::jsi::HostFunctionType &GetHostFunction();

 public:
  static void Finalizer(const v8::WeakCallbackInfo<HostFunctionProxy> &data);

  static void FunctionCallback(const v8::FunctionCallbackInfo<v8::Value> &info);

 private:
  V8Runtime &runtime_;
  v8::Isolate *isolate_;
  facebook::jsi::HostFunctionType hostFunction_;
  v8::Global<v8::Object> weakHandle_;
};

} // namespace rnv8
