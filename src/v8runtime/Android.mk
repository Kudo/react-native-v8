# Copyright (c) Facebook, Inc. and its affiliates.
#
# This source code is licensed under the MIT license found in the
# LICENSE file in the root directory of this source tree.

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := v8runtime

LOCAL_SRC_FILES := $(wildcard $(LOCAL_PATH)/*.cpp)

LOCAL_C_INCLUDES := $(LOCAL_PATH) $(LOCAL_PATH)/..
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)

LOCAL_CFLAGS := -fexceptions -frtti
ifeq ($(APP_OPTIM),release)
  LOCAL_CFLAGS += -O3
endif

LOCAL_SHARED_LIBRARIES := libfolly_json libv8android glog

include $(BUILD_STATIC_LIBRARY)
