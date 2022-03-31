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

class JSIV8ValueConverter {
 private:
  JSIV8ValueConverter() = delete;
  ~JSIV8ValueConverter() = delete;
  JSIV8ValueConverter(JSIV8ValueConverter &&) = delete;

 public:
  static facebook::jsi::Value ToJSIValue(
      v8::Isolate *isolate,
      const v8::Local<v8::Value> &value);

  static v8::Local<v8::Value> ToV8Value(
      const V8Runtime &runtime,
      const facebook::jsi::Value &value);

  static v8::Local<v8::String> ToV8String(
      const V8Runtime &runtime,
      const facebook::jsi::String &string);

  static v8::Local<v8::String> ToV8String(
      const V8Runtime &runtime,
      const facebook::jsi::PropNameID &propName);

  static v8::MaybeLocal<v8::String> ToV8String(
      const V8Runtime &runtime,
      const std::shared_ptr<const facebook::jsi::Buffer> &buffer);

  static v8::Local<v8::Symbol> ToV8Symbol(
      const V8Runtime &runtime,
      const facebook::jsi::Symbol &symbol);

  static v8::Local<v8::Object> ToV8Object(
      const V8Runtime &runtime,
      const facebook::jsi::Object &object);

  static v8::Local<v8::Array> ToV8Array(
      const V8Runtime &runtime,
      const facebook::jsi::Array &array);

  static v8::Local<v8::Function> ToV8Function(
      const V8Runtime &runtime,
      const facebook::jsi::Function &function);

  static facebook::jsi::PropNameID ToJSIPropNameID(
      const V8Runtime &runtime,
      const v8::Local<v8::Name> &property);

  static std::string ToSTLString(const v8::String::Utf8Value &string);

  static std::string ToSTLString(
      v8::Isolate *isolate,
      const v8::Local<v8::Value> &string);
};

} // namespace rnv8
