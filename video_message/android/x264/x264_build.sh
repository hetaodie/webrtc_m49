 #!/bin/bash
NDK=/home/jiarong/dev/tools/android-ndk-r10e
SYSROOT=$NDK/platforms/android-15/arch-arm/  
TOOLCHAIN=$NDK/toolchains/arm-linux-androideabi-4.9/prebuilt/linux-x86_64 
 function build_one
 {
 ./configure \
 --cross-prefix=$TOOLCHAIN/bin/arm-linux-androideabi- \
 --sysroot="$SYSROOT" \
 --host=arm-linux \
 --prefix=/home/jiarong/dev/x264 \
 --enable-pic \
 --enable-shared \
 --enable-static \
 --disable-cli
 --disable-asm
 make
 make install
 }
 build_one
