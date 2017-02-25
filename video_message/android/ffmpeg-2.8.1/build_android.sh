#!/bin/bash  
NDK=/home/jiarong/dev/tools/android-ndk-r10e
SYSROOT=$NDK/platforms/android-15/arch-arm/  
TOOLCHAIN=$NDK/toolchains/arm-linux-androideabi-4.9/prebuilt/linux-x86_64  
  
function build_one  
{  
./configure \
    --prefix=$PREFIX \
    --disable-shared \
    --enable-static \
    --enable-gpl \
    --enable-version3 \
    --disable-programs \
    --disable-ffmpeg \
    --disable-ffplay \
    --disable-ffprobe \
    --disable-ffserver \
    --disable-avdevice \
    --disable-network \
    --disable-avfilter \
    --disable-doc \
    --disable-symver \
    --disable-avformat \
    --disable-swresample \
    --disable-postproc \
    --disable-htmlpages \
    --disable-manpages \
    --disable-podpages \
    --disable-txtpages \
    --disable-symver \
    --disable-w32threads \
    --disable-lsp \
    --disable-mdct \
    --disable-rdft \
    --disable-fft \
    --disable-faan \
    --disable-pixelutils \
    --disable-everything \
    --cross-prefix=$TOOLCHAIN/bin/arm-linux-androideabi- \
    --target-os=linux \
    --arch=arm \
    --enable-cross-compile \
    --enable-pthreads \
    --enable-small \
    --enable-parser=h264 \
    --enable-decoder=h264 \
    --disable-debug \
    --sysroot=$SYSROOT \
    $ADDITIONAL_CONFIGURE_FLAG
make clean
make
make install
}  
CPU=arm  
PREFIX=$(pwd)/android/$CPU  
ADDI_CFLAGS="-marm"  
build_one
