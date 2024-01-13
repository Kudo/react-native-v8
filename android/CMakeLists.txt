#
# Copyright (c) Software Mansion <swmansion.com>.
# Copyright (c) Kudo Chien.
#
# This source code is licensed under the MIT license found in the
# LICENSE file in the root directory of this source tree.
#

cmake_minimum_required(VERSION 3.13)
project(react-native-v8)

set(CMAKE_VERBOSE_MAKEFILE ON)
if(${REACT_NATIVE_TARGET_VERSION} GREATER_EQUAL 73)
    add_compile_definitions(REACT_NATIVE_0_73)
    set (CMAKE_CXX_STANDARD 20)
else()
    set (CMAKE_CXX_STANDARD 17)
endif()

string(APPEND CMAKE_CXX_FLAGS " -DREACT_NATIVE_TARGET_VERSION=${REACT_NATIVE_TARGET_VERSION} -fexceptions -fno-omit-frame-pointer -frtti -Wno-sign-compare -fstack-protector-all")

include("${REACT_NATIVE_DIR}/ReactAndroid/cmake-utils/folly-flags.cmake")
add_compile_options(${folly_FLAGS})

# if(${NATIVE_DEBUG})
#   string(APPEND CMAKE_C_FLAGS_DEBUG " -g")
#   string(APPEND CMAKE_CXX_FLAGS_DEBUG " -g")
# endif()

if(${ANDROID_ABI} STREQUAL "arm64-v8a" OR ${ANDROID_ABI} STREQUAL "x86_64")
  string(APPEND CMAKE_CXX_FLAGS " -DV8_COMPRESS_POINTERS")
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE INTERNAL "")

set(PACKAGE_NAME "v8executor")
set(SRC_DIR "${CMAKE_SOURCE_DIR}/src/main/cpp")

set(V8RUNTIME_COMMON_DIR "${CMAKE_SOURCE_DIR}/../src/v8runtime")
file(GLOB SOURCES_V8RUNTIME  "${V8RUNTIME_COMMON_DIR}/*.cpp")

add_library(
  ${PACKAGE_NAME}
  SHARED
  ${SOURCES_V8RUNTIME}
  "${SRC_DIR}/V8ExecutorFactory.cpp"
  "${SRC_DIR}/OnLoad.cpp"
)

# includes

target_include_directories(
  ${PACKAGE_NAME}
  PRIVATE
  "${V8RUNTIME_COMMON_DIR}"
  "${REACT_NATIVE_DIR}/ReactCommon/jsiexecutor"
  "${V8_ANDROID_DIR}/dist/include"
)

# find libraries

file(GLOB LIBV8_DIR "${SO_DIR}/v8/jni/${ANDROID_ABI}")

find_library(
  LOG_LIB
  log
)

find_package(ReactAndroid REQUIRED CONFIG)

find_package(fbjni REQUIRED CONFIG)

find_library(
  V8_ANDROID_LIB
  v8android
  PATHS ${LIBV8_DIR}
  NO_CMAKE_FIND_ROOT_PATH
)

# reactnative_internal_static
file(GLOB INCLUDE_RN_JSIREACT_CPP  "${REACT_NATIVE_DIR}/ReactCommon/jsiexecutor/jsireact/*.cpp")
file(GLOB INCLUDE_RN_REACTPERFLOGGER_CPP  "${REACT_NATIVE_DIR}/ReactCommon/reactperflogger/reactperflogger/*.cpp")

add_library(
  reactnative_internal_static
  STATIC
  "${INCLUDE_RN_JSIREACT_CPP}"
  "${INCLUDE_RN_REACTPERFLOGGER_CPP}"
  "${REACT_NATIVE_DIR}/ReactCommon/cxxreact/JSExecutor.cpp"
)

target_include_directories(
  reactnative_internal_static
  PRIVATE
  "${REACT_NATIVE_DIR}/ReactCommon/jsiexecutor"
  "${REACT_NATIVE_DIR}/ReactCommon/reactperflogger"
)

target_link_libraries(
  reactnative_internal_static
  ReactAndroid::folly_runtime
  ReactAndroid::glog
  ReactAndroid::jsi
  ReactAndroid::reactnativejni
)

# link to shared libraries

target_link_libraries(
  ${PACKAGE_NAME}
  ${LOG_LIB}
  android
  fbjni::fbjni
  ReactAndroid::folly_runtime
  ReactAndroid::glog
  ReactAndroid::jsi
  ReactAndroid::jsinspector
  ReactAndroid::reactnativejni
  ReactAndroid::runtimeexecutor
  reactnative_internal_static
  ${V8_ANDROID_LIB}
)
