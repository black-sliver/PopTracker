#!/bin/bash

SDL_SRC=SDL2-2.32.10
IMAGE_SRC=SDL2_image-2.8.8
TTF_SRC=SDL2_ttf-2.24.0

# Helper to cross compile SDL libs for Windows on Linux
# 1. install cross toolchain
# 2. download and unpack sdl, sdl_image and sdl_ttf source
# 3. set folder names above
# 4. optional: disable dynapi since we don't want it (read below)
# 5. run this script
# 6. copy resulting libs and headers into win32-lib

# IMPORTANT: dynapi has to be disabled in SDL2-*/src/dynapi/SDL_dynapi.h
#            loadso is now required for hardware renderers
# TODO: disable dynapi using sed?
# TODO: automate copying of output

HOST="i386-linux"
TARGET_WIN32="i686-w64-mingw32"
TARGET_WIN64="x86_64-w64-mingw32"
OUTPUT_FLAGS="--enable-static --disable-shared --prefix=/tmp/out/" # --with-gnu-ld"
# TODO: decide what to do with webp
IMAGE_FEATURE_FLAGS="--disable-sdltest --disable-stb-image --enable-bmp --enable-gif --enable-jpg --enable-png --disable-avif --disable-jpg-shared --disable-save-jpg --disable-jxl --disable-jxl-shared --disable-lbm --disable-pcx --disable-png-shared --disable-save-png --disable-pnm --disable-svg --disable-tga --disable-tif --disable-tif-shared --disable-xcf --disable-xpm --disable-xv --disable-goi"
TTF_FEATURE_FLAGS="--disable-sdltest --disable-freetypetest"
SDL_FEATURE_FLAGS="--disable-alsatest --disable-esdtest --disable-audio --disable-joystick --disable-haptic --disable-sensor --disable-power --disable-filesystem --disable-cpuinfo --disable-jack --disable-esd --disable-pipewire --disable-pulseaudio --disable-arts --disable-nas --disable-sndio --disable-fusionsound --disable-diskaudio --disable-dummyaudio --disable-libsamplerate --disable-libudev"
#RELEASE_FLAGS="-ffunction-sections -fdata-sections -Wl,--gc-sections -Os -s"
RELEASE_FLAGS="-ffunction-sections -fdata-sections -Os"

build() {
    [ -z "$2" ] && exit 1
    SRC="$1"
    TARGET="$3"
    FEATURE_FLAGS="$4"
    DST="build/$TARGET/$2"

    export PREFIX="/usr/$TARGET"
    INCLUDE="$PREFIX/include"
    CROSS_FLAGS="--host=$TARGET --target=$TARGET --build=$HOST --includedir=$INCLUDE --oldincludedir=$INCLUDE"
    CONFIGURE_FLAGS="$CROSS_FLAGS $OUTPUT_FLAGS $FEATURE_FLAGS"
    TOOLCHAIN="$TARGET-"
    export CC=${TOOLCHAIN}gcc
    export CXX=${TOOLCHAIN}g++
    export LD=${TOOLCHAIN}ld 
    export AR=${TOOLCHAIN}ar
    export AS=${TOOLCHAIN}as
    export NM=${TOOLCHAIN}nm
    export STRIP=${TOOLCHAIN}strip
    export RANLIB=${TOOLCHAIN}ranlib
    export DLLTOOL=${TOOLCHAIN}dlltool
    export OBJDUMP=${TOOLCHAIN}objdump
    export RESCOMP=${TOOLCHAIN}windres
    export CFLAGS="$RELEASE_FLAGS" #-DUSE_NO_MINGW_SETJMP_TWO_ARGS"

    echo "$SRC -> $DST"
    if [ -d "$DST" ]; then rm -R "$DST" ; fi
    mkdir -p "$DST"
    cd "$DST"
    ../../../$SRC/configure $CONFIGURE_FLAGS
    make -j15
    cd ../../..
}

build $SDL_SRC "sdl" $TARGET_WIN64  "$SDL_FEATURE_FLAGS"
build $IMAGE_SRC "sdl2_image" $TARGET_WIN64 "$IMAGE_FEATURE_FLAGS"
build $TTF_SRC "sdl2_ttf" $TARGET_WIN64 "$TTF_FEATURE_FLAGS"

build $SDL_SRC "sdl" $TARGET_WIN32 "$SDL_FEATURE_FLAGS"
build $IMAGE_SRC "sdl2_image" $TARGET_WIN32 "$IMAGE_FEATURE_FLAGS"
build $TTF_SRC "sdl2_ttf" $TARGET_WIN32 "$TTF_FEATURE_FLAGS"

