#!/bin/bash -e
source $(dirname $0)/env.sh

######################################################################################
# [0] Patch React Native source
######################################################################################

rm -rf $BUILD_DIR
git clone --depth=1 --branch ${RN_VERSION} https://github.com/facebook/react-native.git $BUILD_DIR

PATCHSET=(
  # Patch React Native build to support v8runtime
  "build_with_v8.patch"
)

cp -Rf $SRC_DIR/v8runtime $BUILD_DIR/ReactCommon/jsi/
cp -Rf $SRC_DIR/androidexecutor $BUILD_DIR/ReactAndroid/src/main/java/com/facebook/v8
cp -Rf $SRC_DIR/sowrapper $BUILD_DIR/ReactAndroid/src/main/jni/third-party/v8

for patch in "${PATCHSET[@]}"
do
    printf "### Patch set: $patch\n"
    patch -d $BUILD_DIR -p1 < $PATCHES_DIR/$patch
done

######################################################################################
# [1] Build
######################################################################################

cd $BUILD_DIR
yarn
./gradlew :ReactAndroid:installArchives

mkdir -p $DIST_DIR
cp -Rf $BUILD_DIR/android/* $DIST_DIR
