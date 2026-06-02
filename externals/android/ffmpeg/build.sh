#!/bin/bash
make clean
set -e
archbit=64

OPENSSL=/Users/lifengshan/G/work/compile/openssl2/openssl/output
LIBSRT=/Users/lifengshan/G/work/compile/libsrt
LIBX264=/Users/lifengshan/G/work/compile/x264/x264/android

if [ $archbit -eq 64 ];then
echo "build for 64bit"
ARCH=aarch64
CPU=armv8-a
API=21
PLATFORM=aarch64
ANDROID=android
CFLAGS="-fPIC"
LDFLAGS=""
LLIB=android-arm64
#pkgpath=/Users/lifengshan/G/work/compile/libsrt/install64/lib/pkgconfig

else
echo "build for 32bit"
ARCH=arm
CPU=armv7-a
API=21
PLATFORM=armv7a
ANDROID=androideabi
CFLAGS="-mfloat-abi=softfp -march=$CPU"
LDFLAGS="-Wl,--fix-cortex-a8"
LLIB=android-arm

#pkgpath=/Users/lifengshan/G/work/compile/libsrt/install32/lib/pkgconfig

fi
 
export NDK=/Users/lifengshan/Library/Android/sdk/ndk/25.1.8937393
export TOOLCHAIN=$NDK/toolchains/llvm/prebuilt/darwin-x86_64/bin
export SYSROOT=$NDK/toolchains/llvm/prebuilt/darwin-x86_64/sysroot
export CROSS_PREFIX=$TOOLCHAIN/llvm-
export CC=$TOOLCHAIN/$PLATFORM-linux-$ANDROID$API-clang
export CXX=$TOOLCHAIN/$PLATFORM-linux-$ANDROID$API-clang++
export PREFIX=$(pwd)/android/$CPU


#export PKG_CONFIG_PATH=$pkgpath:/Users/lifengshan/G/work/compile/openssl2/openssl/output/lib/android-arm64/pkgconfig
# pkg-config --modversion srt
# pkg-config --libs srt

# pkg-config --modversion libssl
# pkg-config --libs libssl

#echo $LIBSRT/lib/$LLIB
echo build_android

function build_android {
    ./configure \
    --prefix=$PREFIX \
    --cross-prefix=$CROSS_PREFIX \
    --target-os=android \
    --arch=$ARCH \
    --cpu=$CPU \
    --cc=$CC \
    --cxx=$CXX \
    --nm=$TOOLCHAIN/llvm-nm \
    --strip=$TOOLCHAIN/llvm-strip \
    --enable-cross-compile \
    --sysroot=$SYSROOT \
    --extra-cflags="$CFLAGS" \
    --extra-ldflags="$LDFLAGS" \
    --extra-ldexeflags=-pie \
    --enable-runtime-cpudetect \
    --enable-pic \
  --enable-static \
  --disable-shared \
  --disable-ffmpeg \
  --disable-ffplay \
  --disable-ffprobe \
  --enable-avfilter \
  --enable-small \
  --disable-armv5te \
  --disable-armv6 \
  --disable-armv6t2 \
  --enable-vfp \
  --enable-neon \
  --enable-pic \
  --disable-encoders \
  --disable-filters \
  --disable-vulkan \
  --enable-asm \
  --enable-yasm \
  --disable-debug \
  --enable-avdevice \
  --enable-postproc \
  --disable-encoders \
  --enable-encoder=mjpeg \
  --enable-encoder=png \
  --enable-encoder=aac \
  --disable-hwaccels \
  --disable-muxers \
  --enable-muxer=mov \
  --enable-muxer=mp4 \
  --enable-muxer=flv \
  --disable-decoders \
  --enable-decoder=mpeg4 \
  --enable-decoder=h264 \
  --enable-decoder=hevc \
  --enable-decoder=flv \
  --enable-decoder=aac \
  --enable-decoder=ac3 \
  --enable-decoder=mp3 \
  --enable-decoder=opus \
  --disable-demuxers \
  --enable-demuxer=hls \
  --enable-demuxer=mpegts \
  --enable-demuxer=mov \
  --enable-demuxer=flv \
  --enable-demuxer=live_flv \
  --enable-demuxer=rtsp \
  --enable-demuxer=rtmp \
  --enable-demuxer=mp3 \
  --enable-demuxer=matroska \
  --enable-demuxer=gif \
  --enable-demuxer=aac \
  --enable-protocol=rtmp \
  --enable-libsrt \
  --enable-openssl \
  --enable-jni \
  --enable-mediacodec \
  --enable-decoder=h264_mediacodec \
  --enable-decoder=hevc_mediacodec \
  --enable-decoder=mpeg4_mediacodec \
  --enable-libx264 \
  --enable-encoder=libx264 \
  --enable-encoder=libx264rgb \
  --enable-gpl \
  --disable-x86asm \
  --extra-cflags="-I$LIBSRT/include -I$OPENSSL/include -I$LIBX264/armv8-a/include -fPIC -fPIE -pie" \
  --extra-ldflags="-L$LIBSRT/lib/$LLIB -L$OPENSSL/lib/$LLIB -I$LIBX264/armv8-a/lib" \
    $ADDITIONAL_CONFIGURE_FLAG
 
    make
    make install
}

#echo $OPENSSL/lib/$LLIB

build_android


#--extra-libs="-lsrt -lssl -lcrypto" \