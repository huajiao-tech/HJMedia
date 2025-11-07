#!/bin/bash

PROJECT_DIR=$(dirname "$0")
SRC_DIR=$(realpath "$PROJECT_DIR")
BUILD_DIR="$PROJECT_DIR/build_harmony"

mkdir "$BUILD_DIR"

echo "$PROJECT_DIR"
echo "$BUILD_DIR"
echo "$SRC_DIR"

export TOOL_HOME=/Applications/DevEco-Studio.app/Contents
export HARMONY_SDK_PATH="$TOOL_HOME/sdk/default/openharmony/native"
export HARMONY_CMAKE_PATH="$HARMONY_SDK_PATH/build-tools/cmake/bin"
export HARMONY_NINJA_PATH="$HARMONY_CMAKE_PATH"

export PATH="$HARMONY_CMAKE_PATH:$HARMONY_NINJA_PATH:$PATH"

if ! command -v cmake &> /dev/null; then
    echo "错误: 找不到 cmake"
    echo "请检查路径: $HARMONY_CMAKE_PATH"
    exit 1
fi

$HARMONY_CMAKE_PATH/cmake -G Ninja \
-DCMAKE_TOOLCHAIN_FILE=$HARMONY_SDK_PATH/build/cmake/ohos.toolchain.cmake \
-DOHOS_ARCH="arm64-v8a" \
-DOHOS_PLATFORM="OHOS" \
-DCMAKE_BUILD_TYPE=Release \
-S $SRC_DIR \
-B $BUILD_DIR

cmake --build $BUILD_DIR

echo $BUILD_DIR
echo $SRC_DIR

# cp -f "$BUILD_DIR/output/libs/libjplayer.so" "$SRC_DIR/examples/Harmony/hmplayer/libs/arm64-v8a/"
# cp -f "$BUILD_DIR/output/libs/libpublisher.so" "$SRC_DIR/examples/Harmony/hmplayer/libs/arm64-v8a/"