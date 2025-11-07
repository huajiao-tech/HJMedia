#!/bin/bash

# Build and package the SDK for iOS
# Assumes that you are in the SDK/iOS directory

SDKBUILDDIR=`pwd`

OUT_DIR=build_mos
BUILD_DIR=$SDKBUILDDIR/$OUT_DIR
# Clean up files from previous builds
echo Cleaning previous builds...
if [ "$1" = "clean" ];then
	rm -rf $BUILD_DIR
fi

# Configure with CMake
mkdir -p $BUILD_DIR
cd $OUT_DIR

cmake -G Xcode ../ -DCMAKE_PREFIX_PATH=$BUILD_DIR -DCMAKE_OSX_ARCHITECTURES=x86_64

cd ..

echo Done!
