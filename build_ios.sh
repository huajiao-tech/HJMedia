#!/bin/bash

# Build and package the SDK for iOS
# Assumes that you are in the SDK/iOS directory

SDKBUILDDIR=`pwd`

OUT_DIR=build
# Clean up files from previous builds
echo Cleaning previous builds...
if [ "$1" = "clean" ];then
	rm -rf $SDKBUILDDIR/$OUT_DIR
fi

# Configure with CMake
mkdir -p $SDKBUILDDIR/$OUT_DIR
cd $OUT_DIR

cmake -G Xcode \
 -DCMAKE_TOOLCHAIN_FILE=cmake/ios.toolchain.cmake \
 -DPLATFORM=OS64 \
 -DENABLE_BITCODE=0 \
 -DDEPLOYMENT_TARGET=12.0 \
 -DENABLE_ARC=ON \
 -DENABLE_VISIBILITY=OFF \
 -DENABLE_STRICT_TRY_COMPILE=OFF \
 -DCMAKE_CXX_FLAGS=-Wno-c++11-narrowing \
 -DCMAKE_SHARED_LINKER_FLAGS=-Wl,-Bsymbolic \
 ../

cd ..

echo Done!
