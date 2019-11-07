# Copyright (c) Facebook, Inc. and its affiliates.
#
# This source code is licensed under the MIT license found in the
# LICENSE file in the root directory of this source tree.

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
REACT_NATIVE := $(LOCAL_PATH)/../../../../../../../..

LOCAL_MODULE := v8executor

LOCAL_SRC_FILES := $(wildcard $(LOCAL_PATH)/*.cpp)

LOCAL_C_INCLUDES := $(LOCAL_PATH) $(REACT_NATIVE)/ReactCommon/jsi $(REACT_NATIVE)/node_modules/react-native-v8/dist/include

LOCAL_CPP_FEATURES := exceptions

LOCAL_STATIC_LIBRARIES := libjsireact libjsi v8runtime
LOCAL_SHARED_LIBRARIES := libfolly_json libfb libreactnativejni libv8android

include $(BUILD_SHARED_LIBRARY)
