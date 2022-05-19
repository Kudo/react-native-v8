/*
 * Copyright (c) Kudo Chien.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include "V8PointerValue.h"

namespace rnv8 {

V8PointerValue::V8PointerValue(
    v8::Isolate *isolate,
    const v8::Local<v8::Value> &value)
    : isolate_(isolate), value_(isolate, value) {}

V8PointerValue::V8PointerValue(
    v8::Isolate *isolate,
    v8::Global<v8::Value> &&value)
    : isolate_(isolate), value_(std::move(value)) {}

V8PointerValue::~V8PointerValue() {}

v8::Local<v8::Value> V8PointerValue::Get(v8::Isolate *isolate) const {
  v8::EscapableHandleScope scopedHandle(isolate);
  return scopedHandle.Escape(value_.Get(isolate));
}

// static
V8PointerValue *V8PointerValue::createFromOneByte(
    v8::Isolate *isolate,
    const char *str,
    size_t length) {
  v8::HandleScope scopedHandle(isolate);
  v8::Local<v8::String> v8String;
  if (!v8::String::NewFromOneByte(
           isolate,
           reinterpret_cast<const uint8_t *>(str),
           v8::NewStringType::kNormal,
           static_cast<int>(length))
           .ToLocal(&v8String)) {
    return nullptr;
  }
  return new V8PointerValue(isolate, v8String);
}

// static
V8PointerValue *V8PointerValue::createFromUtf8(
    v8::Isolate *isolate,
    const uint8_t *str,
    size_t length) {
  v8::HandleScope scopedHandle(isolate);
  v8::Local<v8::String> v8String;
  if (!v8::String::NewFromUtf8(
           isolate,
           reinterpret_cast<const char *>(str),
           v8::NewStringType::kNormal,
           static_cast<int>(length))
           .ToLocal(&v8String)) {
    return nullptr;
  }
  return new V8PointerValue(isolate, v8String);
}

void V8PointerValue::invalidate() {
  {
    v8::Locker locker(isolate_);
    v8::Isolate::Scope scopedIsolate(isolate_);
    value_.Reset();
  }
  delete this;
}

} // namespace rnv8
