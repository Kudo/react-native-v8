#
# Copyright (c) Software Mansion <swmansion.com>.
# Copyright (c) Kudo Chien.
#
# This source code is licensed under the MIT license found in the
# LICENSE file in the root directory of this source tree.
#

cmake_minimum_required(VERSION 3.5.1)
project(react-native-v8)

set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_FLAGS "-DFOLLY_NO_CONFIG=1 -DFOLLY_HAVE_CLOCK_GETTIME=1 -DFOLLY_HAVE_MEMRCHR=1 -DFOLLY_USE_LIBCPP=1 -DFOLLY_MOBILE=1 -DREACT_NATIVE_TARGET_VERSION=${REACT_NATIVE_TARGET_VERSION} -fexceptions -fno-omit-frame-pointer -frtti -Wno-sign-compare")

# if(${NATIVE_DEBUG})
#   set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -g")
#   set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g")
# endif()

if(${ANDROID_ABI} STREQUAL "arm64-v8a" OR ${ANDROID_ABI} STREQUAL "x86_64")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DV8_COMPRESS_POINTERS")
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE INTERNAL "")

set(PACKAGE_NAME "v8executor")
set(SRC_DIR "${CMAKE_SOURCE_DIR}/src/main/cpp")

set(FBJNI_HEADERS_DIR "${RN_DIR}/ReactAndroid/src/main/jni/first-party/fbjni/headers")

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

file(GLOB LIBFBJNI_INCLUDE_DIR ${FBJNI_HEADERS_DIR})

target_include_directories(
  ${PACKAGE_NAME}
  PRIVATE
  "${V8RUNTIME_COMMON_DIR}"
  "${LIBFBJNI_INCLUDE_DIR}"
  "${BUILD_DIR}/third-party-ndk/boost/boost_${BOOST_VERSION}"
  "${BUILD_DIR}/third-party-ndk/double-conversion"
  "${BUILD_DIR}/third-party-ndk/folly"
  "${BUILD_DIR}/third-party-ndk/glog/exported"
  "${RN_DIR}/ReactAndroid/src/main/jni"
  "${RN_DIR}/ReactCommon"
  "${RN_DIR}/ReactCommon/jsi"
  "${RN_DIR}/ReactCommon/jsiexecutor"
  "${V8_ANDROID_DIR}/dist/include"
)

# find libraries

file(GLOB LIBRN_DIR "${SO_DIR}/react/jni/${ANDROID_ABI}")
file(GLOB LIBV8_DIR "${SO_DIR}/v8/jni/${ANDROID_ABI}")

find_library(
  LOG_LIB
  log
)
if(${REACT_NATIVE_TARGET_VERSION} LESS 69)
  find_library(
    FOLLY_LIB
    folly_json
    PATHS ${LIBRN_DIR}
    NO_CMAKE_FIND_ROOT_PATH
  )
else()
  find_library(
    FOLLY_LIB
    folly_runtime
    PATHS ${LIBRN_DIR}
    NO_CMAKE_FIND_ROOT_PATH
  )
endif()
find_library(
  REACT_NATIVE_JNI_LIB
  reactnativejni
  PATHS ${LIBRN_DIR}
  NO_CMAKE_FIND_ROOT_PATH
)
find_library(
  GLOG_LIB
  glog
  PATHS ${LIBRN_DIR}
  NO_CMAKE_FIND_ROOT_PATH
)
find_library(
  FBJNI_LIB
  fbjni
  PATHS ${LIBRN_DIR}
  NO_CMAKE_FIND_ROOT_PATH
)
find_library(
  JSI_LIB
  jsi
  PATHS ${LIBRN_DIR}
  NO_CMAKE_FIND_ROOT_PATH
)
find_library(
  JSINSPECTOR_LIB
  jsinspector
  PATHS ${LIBRN_DIR}
  NO_CMAKE_FIND_ROOT_PATH
)
find_library(
  V8_ANDROID_LIB
  v8android
  PATHS ${LIBV8_DIR}
  NO_CMAKE_FIND_ROOT_PATH
)

# reactnative_internal_static
file(GLOB INCLUDE_RN_JSIREACT_CPP  "${RN_DIR}/ReactCommon/jsiexecutor/jsireact/*.cpp")
file(GLOB INCLUDE_RN_REACTPERFLOGGER_CPP  "${RN_DIR}/ReactCommon/reactperflogger/reactperflogger/*.cpp")

add_library(
  reactnative_internal_static
  STATIC
  "${INCLUDE_RN_JSIREACT_CPP}"
  "${INCLUDE_RN_REACTPERFLOGGER_CPP}"
  "${RN_DIR}/ReactCommon/cxxreact/JSExecutor.cpp"
)

target_include_directories(
  reactnative_internal_static
  PRIVATE
  "${BUILD_DIR}/third-party-ndk/boost/boost_${BOOST_VERSION}"
  "${BUILD_DIR}/third-party-ndk/double-conversion"
  "${BUILD_DIR}/third-party-ndk/folly"
  "${BUILD_DIR}/third-party-ndk/glog/exported"
  "${RN_DIR}/ReactCommon"
  "${RN_DIR}/ReactCommon/jsi"
  "${RN_DIR}/ReactCommon/jsiexecutor"
  "${RN_DIR}/ReactCommon/jsinspector"
  "${RN_DIR}/ReactCommon/reactperflogger"
)

target_link_libraries(
  reactnative_internal_static
  ${FOLLY_LIB}
)

# link to shared libraries

set_target_properties(${PACKAGE_NAME} PROPERTIES LINKER_LANGUAGE CXX)

target_link_libraries(
  ${PACKAGE_NAME}
  ${LOG_LIB}
  ${JSI_LIB}
  ${JSINSPECTOR_LIB}
  ${GLOG_LIB}
  ${FBJNI_LIB}
  ${FOLLY_LIB}
  ${REACT_NATIVE_JNI_LIB}
  ${V8_ANDROID_LIB}
  reactnative_internal_static
  android
)
