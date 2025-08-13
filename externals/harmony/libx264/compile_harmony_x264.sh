#!/bin/bash
make clean
set -e

export OHOS_SDK=/home/chenyajie/linux
export AS=${OHOS_SDK}/native/llvm/bin/llvm-as
export CC=${OHOS_SDK}/native/llvm/bin/aarch64-linux-ohos-clang
export CXX=${OHOS_SDK}/native/llvm/bin/aarch64-linux-ohos-clang++
export LD=${OHOS_SDK}/native/llvm/bin/ld.lld
export STRIP=${OHOS_SDK}/native/llvm/bin/llvm-strip
export RANLIB=${OHOS_SDK}/native/llvm/bin/llvm-ranlib
export OBJDUMP=${OHOS_SDK}/native/llvm/bin/llvm-objdump
export OBJCOPY=${OHOS_SDK}/native/llvm/bin/llvm-objcopy
export NM=${OHOS_SDK}/native/llvm/bin/llvm-nm
export AR=${OHOS_SDK}/native/llvm/bin/llvm-ar
export CFLAGS="-DOHOS_NDK -fPIC -D__MUSL__=1"
export CXXFLAGS="-DOHOS_NDK -fPIC -D__MUSL__=1"
export PREFIX="${PWD}/ohbuild"

echo build_ohos

function build_ohos {

    ./configure \
    --enable-static \
    --host=aarch64-linux \
    --sysroot="${OHOS_SDK}/native/sysroot" \
    --disable-asm \
    --prefix=${PREFIX} \

    make
    make install
}

build_ohos