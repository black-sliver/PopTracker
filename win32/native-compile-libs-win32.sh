#!/bin/bash

SDL_SRC=SDL2-2.30.10
SDL_URL="https://github.com/libsdl-org/SDL/releases/download/release-2.30.10/SDL2-2.30.10.tar.gz"
IMAGE_SRC=SDL2_image-2.8.4
IMAGE_URL="https://github.com/libsdl-org/SDL_image/releases/download/release-2.8.4/SDL2_image-2.8.4.tar.gz"
TTF_SRC=SDL2_ttf-2.22.0
TTF_URL="https://github.com/libsdl-org/SDL_ttf/releases/download/release-2.22.0/SDL2_ttf-2.22.0.tar.gz"

# IMPORTANT: dynapi has to be disabled in SDL2-*/src/dynapi/SDL_dynapi.h
#            loadso is now required for hardware renderers
# TODO: disable dynapi using sed?
# TODO: automate copying of output

ARCH=`gcc -dumpmachine | sed "s/-.*$//"`
OUTPUT_FLAGS="--enable-static --disable-shared" # --with-gnu-ld"
IMAGE_FEATURE_FLAGS="--disable-sdltest --disable-freetypetest"
TTF_FEATURE_FLAGS="--disable-sdltest --disable-avif --disable-jpg-shared --disable-save-jpg --disable-jxl --disable-jxl-shared --disable-lbm --disable-pcx --disable-png-shared --disable-save-png --disable-pnm --disable-svg --disable-tga --disable-tif --disable-tif-shared --disable-xcf --disable-xpm --disable-xv --disable-goi"
SDL_FEATURE_FLAGS="--disable-alsatest --disable-esdtest --disable-audio --disable-joystick --disable-haptic --disable-sensor --disable-power --disable-filesystem --disable-cpuinfo --disable-jack --disable-esd --disable-pipewire --disable-pulseaudio --disable-arts --disable-nas --disable-sndio --disable-fusionsound --disable-diskaudio --disable-dummyaudio --disable-libsamplerate --disable-libudev"
#RELEASE_FLAGS="-ffunction-sections -fdata-sections -Wl,--gc-sections -Os -s"
RELEASE_FLAGS="-ffunction-sections -fdata-sections -Os"

build() {
    [ -z "$2" ] && exit 1
    SRC="$1"
    TARGET="$3"
    FEATURE_FLAGS="$4"
    BUILD="build/$TARGET/$2"

    echo "Building $2 on $ARCH"

    CONFIGURE_FLAGS="$OUTPUT_FLAGS $FEATURE_FLAGS"
    echo "$SRC -> $BUILD"
    if [ -d "$BUILD" ]; then rm -R "$BUILD" ; fi
    mkdir -p "$BUILD"
    cd "$BUILD"
    ../../../$SRC/configure $CONFIGURE_FLAGS
    make -j4
    cd ../../..
}

# Download missing sources
if [ ! -d $SDL_SRC ]; then
    wget "$SDL_URL"
    tar -xzvf "$SDL_SRC.tar.gz"
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

build $SDL_SRC "sdl" "native" "$SDL_FEATURE_FLAGS"
build $IMAGE_SRC "sdl2_image" "native" "$IMAGE_FEATURE_FLAGS"
build $TTF_SRC "sdl2_ttf" "native" "$TTF_FEATURE_FLAGS"

DST="../win32-lib/$ARCH"
echo "build/... -> $DST"
mkdir -p $DST/include/SDL2
mkdir -p $DST/lib
cp $SDL_SRC/include/*  "$DST/include/SDL2/"
cp $IMAGE_SRC/include/*  "$DST/include/SDL2/"
cp $TTF_SRC/*.h  "$DST/include/SDL2/"
cp build/native/sdl/include/* "$DST/include/SDL2/"
cp build/native/sdl/build/.libs/* "$DST/lib/"
cp build/native/sdl/build/*.la "$DST/lib/"
cp build/native/sdl2_image/.libs/* "$DST/lib/"
cp build/native/sdl2_image/*.la "$DST/lib/"
cp build/native/sdl2_ttf/.libs/* "$DST/lib/"
cp build/native/sdl2_ttf/*.la "$DST/lib/"
ls $DST/*
