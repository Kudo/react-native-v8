#!/bin/bash -e
echo "ooxx0"
source $(dirname $0)/env.sh

echo "ooxx1"
######################################################################################
# [0] Patch React Native source
######################################################################################

echo "ooxx2"
rm -rf $BUILD_DIR
echo "ooxx3"
git clone --depth=1 --branch ${RN_VERSION} https://github.com/facebook/react-native.git $BUILD_DIR

echo "ooxx4"
PATCHSET=(
  # Patch React Native build to support v8runtime
  "build_with_v8.patch"
)

echo "ooxx5"
cp -Rf $SRC_DIR/v8runtime $BUILD_DIR/ReactCommon/jsi/
cp -Rf $SRC_DIR/androidexecutor $BUILD_DIR/ReactAndroid/src/main/java/com/facebook/v8
cp -Rf $SRC_DIR/sowrapper $BUILD_DIR/ReactAndroid/src/main/jni/third-party/v8

echo "ooxx6"
for patch in "${PATCHSET[@]}"
do
    printf "### Patch set: $patch\n"
    patch -d $BUILD_DIR -p1 < $PATCHES_DIR/$patch
done

######################################################################################
# [1] Build
######################################################################################

echo "ooxx7"
cd $BUILD_DIR
echo "ooxx8"
yarn
echo "ooxx9"
./gradlew :ReactAndroid:installArchives
# NOTE(kudo): Sometimes gradle will not pack *.so (maybe from parallel issues),
# workaround to execute twice
./gradlew :ReactAndroid:installArchives

mkdir -p $DIST_DIR
cp -Rf $BUILD_DIR/android/* $DIST_DIR
