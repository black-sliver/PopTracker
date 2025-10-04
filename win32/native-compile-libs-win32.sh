#!/bin/bash

SDL_SRC=SDL2-2.32.10
SDL_URL="https://github.com/libsdl-org/SDL/releases/download/release-2.32.10/SDL2-2.32.10.tar.gz"
IMAGE_SRC=SDL2_image-2.8.8
IMAGE_URL="https://github.com/libsdl-org/SDL_image/releases/download/release-2.8.8/SDL2_image-2.8.8.tar.gz"
TTF_SRC=SDL2_ttf-2.24.0
TTF_URL="https://github.com/libsdl-org/SDL_ttf/releases/download/release-2.24.0/SDL2_ttf-2.24.0.tar.gz"

# IMPORTANT: dynapi has to be disabled in SDL2-*/src/dynapi/SDL_dynapi.h, see sed -i below.
#            loadso is now required for hardware renderers

ARCH=`gcc -dumpmachine | sed "s/-.*$//"`
OUTPUT_FLAGS="--enable-static --disable-shared" # --with-gnu-ld"
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
    BUILD="build/$TARGET/$2"

    if [ -f "$BUILD/.version" ] && [ `cat "$BUILD/.version"` == "$1" ]; then
        echo "Skipping build $2 on $ARCH, $1 already built"
        return 0
    fi

    echo "Building $2 on $ARCH"
    CONFIGURE_FLAGS="$OUTPUT_FLAGS $FEATURE_FLAGS"
    echo "$SRC -> $BUILD"
    if [ -d "$BUILD" ]; then rm -R "$BUILD" ; fi
    mkdir -p "$BUILD"
    cd "$BUILD"
    ../../../$SRC/configure $CONFIGURE_FLAGS
    make -j4
    echo "$1" > ".version"
    cd ../../..
}

# Download missing sources
if [ ! -d $SDL_SRC ]; then
    wget "$SDL_URL"
    tar -xzvf "$SDL_SRC.tar.gz"
    # disable dynapi
    sed -i 's/#define SDL_DYNAMIC_API 1$/#define SDL_DYNAMIC_API 0/' "$SDL_SRC/src/dynapi/SDL_dynapi.h"
fi
if [ ! -d $IMAGE_SRC ]; then
    wget "$IMAGE_URL"
    tar -xzvf "$IMAGE_SRC.tar.gz"
    cd "$IMAGE_SRC"
    autoreconf -f -i  # for whatever reason this seems to be required
    cd ..
fi
if [ ! -d $TTF_SRC ]; then
    wget "$TTF_URL"
    tar -xzvf "$TTF_SRC.tar.gz"
fi

build $SDL_SRC "sdl" "$ARCH" "$SDL_FEATURE_FLAGS"
build $IMAGE_SRC "sdl2_image" "$ARCH" "$IMAGE_FEATURE_FLAGS"
build $TTF_SRC "sdl2_ttf" "$ARCH" "$TTF_FEATURE_FLAGS"

DST="../win32-lib/$ARCH"
BUILD="build/$ARCH"
echo "$BUILD/... -> $DST"
mkdir -p $DST/include/SDL2
mkdir -p $DST/lib
cp $SDL_SRC/include/*  "$DST/include/SDL2/"
cp $IMAGE_SRC/include/*  "$DST/include/SDL2/"
cp $TTF_SRC/*.h  "$DST/include/SDL2/"
cp $BUILD/sdl/include/* "$DST/include/SDL2/"
cp $BUILD/sdl/build/.libs/* "$DST/lib/"
cp $BUILD/sdl/build/*.la "$DST/lib/"
cp $BUILD/sdl2_image/.libs/* "$DST/lib/"
cp $BUILD/sdl2_image/*.la "$DST/lib/"
cp $BUILD/sdl2_ttf/.libs/* "$DST/lib/"
cp $BUILD/sdl2_ttf/*.la "$DST/lib/"
ls $DST/*
