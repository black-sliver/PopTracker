#!/bin/bash

# script to fetch and build third party sources.
# (c) 2022 sbzappa

CLEAR_BUILD_DIR=no
BUILD_DIR="build"
CLEAR_LIB_DIR=no
LIB_DIR="libs"
DEPLOYMENT_TARGET="10.12"

#WITH_FREETYPE=true  # latest SDL_TTF bundles freetype
#WITH_PNG=true  # only required as dependency of freetype

function usage() {
  echo "Usage: `basename $0` [--build-dir=] [--clear-build-dir] [--lib-dir=] [--clear-lib-dir] [--deployment-target=]"
}

while test $# -gt 0; do
  case "$1" in
  -*=*) optarg=`echo "$1" | sed 's/[-_a-zA-Z0-9]*=//'` ;;
  *) optarg= ;;
  esac

  case $1 in
    --build-dir=*)
      BUILD_DIR=$optarg
      ;;
    --lib-dir=*)
      LIB_DIR=$optarg
      ;;
    --deployment-target=*)
      DEPLOYMENT_TARGET=$optarg
      ;;
    --clear-build-dir)
      CLEAR_BUILD_DIR=yes
      ;;
    --clear-lib-dir)
      CLEAR_LIB_DIR=yes
      ;;
    *)
      usage
      exit 1
      ;;
  esac
  shift
done

SRC_DIR=`dirname $0`
cd $SRC_DIR

LIB_SDL_URL="https://github.com/libsdl-org/SDL.git"
LIB_SDL_IMAGE_URL="https://github.com/libsdl-org/SDL_image.git"
LIB_SDL_TTF_URL="https://github.com/libsdl-org/SDL_ttf.git"
LIB_PNG_URL="https://github.com/pnggroup/libpng.git"
LIB_FREETYPE_URL="https://gitlab.freedesktop.org/freetype/freetype.git"
LIB_OPENSSL_URL="https://github.com/openssl/openssl.git"

LIB_SDL_TAG="release-2.32.10"
LIB_SDL_IMAGE_TAG="release-2.8.8"
LIB_SDL_TTF_TAG="release-2.24.0"
LIB_PNG_TAG="v1.6.54"
LIB_FREETYPE_TAG="VER-2-13-3"
LIB_OPENSSL_TAG="openssl-3.0.19"

LIB_SDL_TARGET="build/.libs/libSDL2-2.0.0.dylib"
LIB_SDL_IMAGE_TARGET=".libs/libSDL2_image-2.0.0.dylib"
LIB_SDL_TTF_TARGET=".libs/libSDL2_ttf-2.0.0.dylib"
LIB_PNG_TARGET=".libs/libpng16.16.dylib"
LIB_FREETYPE_TARGET="objs/.libs/libfreetype.6.dylib"
LIB_OPENSSL_TARGET="libssl.3.dylib"
LIB_CRYPTO_TARGET="libcrypto.3.dylib"

# TODO: decide what to do with webp
LIB_SDL_IMAGE_FEATURE_FLAGS="--disable-sdltest --disable-stb-image --enable-bmp --enable-gif --enable-jpg --enable-png --disable-avif --disable-jpg-shared --disable-save-jpg --disable-jxl --disable-jxl-shared --disable-lbm --disable-pcx --disable-png-shared --disable-save-png --disable-pnm --disable-svg --disable-tga --disable-tif --disable-tif-shared --disable-xcf --disable-xpm --disable-xv --disable-goi"

REGEX="s/^.*\/\([^\/]*\).git$/\1/"

LIB_SDL_DEST_DIR="$BUILD_DIR/`echo $LIB_SDL_URL | sed $REGEX`"
LIB_SDL_IMAGE_DEST_DIR="$BUILD_DIR/`echo $LIB_SDL_IMAGE_URL | sed $REGEX`"
LIB_SDL_TTF_DEST_DIR="$BUILD_DIR/`echo $LIB_SDL_TTF_URL | sed $REGEX`"
LIB_PNG_DEST_DIR="$BUILD_DIR/`echo $LIB_PNG_URL | sed $REGEX`"
LIB_FREETYPE_DEST_DIR="$BUILD_DIR/`echo $LIB_FREETYPE_URL | sed $REGEX`"
LIB_OPENSSL_DEST_DIR="$BUILD_DIR/`echo $LIB_OPENSSL_URL | sed $REGEX`"


DEPLOYMENT_TARGET_FLAG="-mmacosx-version-min=$DEPLOYMENT_TARGET"
CONFIGURE_FLAGS="CXXFLAGS=$DEPLOYMENT_TARGET_FLAG CFLAGS=$DEPLOYMENT_TARGET_FLAG LDFLAGS=$DEPLOYMENT_TARGET_FLAG"

if test $CLEAR_BUILD_DIR = yes ; then
  rm -fr $BUILD_DIR
fi


if test $CLEAR_LIB_DIR = yes ; then
  rm -fr $LIB_DIR
fi

mkdir -p $BUILD_DIR
mkdir -p $LIB_DIR

if [ "$WITH_PNG" ]; then
    # Build Png

    if [ ! -d $LIB_PNG_DEST_DIR ]; then
      git clone --depth 1 --branch $LIB_PNG_TAG $LIB_PNG_URL $LIB_PNG_DEST_DIR || { echo "Could not clone $LIB_PNG_URL" ; exit 1; }
      cd $LIB_PNG_DEST_DIR
    else
      cd $LIB_PNG_DEST_DIR
      git pull
      git checkout $LIB_PNG_TAG
    fi

    ./configure $CONFIGURE_FLAGS
    make -j3

    [ -f "$LIB_PNG_TARGET" ] || { echo "Missing $LIB_PNG_TARGET..." ; exit 1; }

    cp "$LIB_PNG_TARGET" "../../$LIB_DIR"
    cd "../.."
fi

if [ "$WITH_FREETYPE" ]; then
    # Build Freetype

    if [ ! -d $LIB_FREETYPE_DEST_DIR ]; then
      git clone --depth 1 --branch $LIB_FREETYPE_TAG $LIB_FREETYPE_URL $LIB_FREETYPE_DEST_DIR || { echo "Could not clone $LIB_FREETYPE_URL" ; exit 1; }
      cd $LIB_FREETYPE_DEST_DIR
    else
      cd $LIB_FREETYPE_DEST_DIR
      git pull
      git checkout $LIB_FREETYPE_TAG
    fi

    ./autogen.sh
    ./configure $CONFIGURE_FLAGS --with-harfbuzz=no --with-brotli=no --with-png=yes
    make -j3

    [ -f $LIB_FREETYPE_TARGET ] || { echo "Missing $LIB_FREETYPE_TARGET..." ; exit 1; }

    cp "$LIB_FREETYPE_TARGET" "../../$LIB_DIR"
    cd "../.."
fi

# Build SDL

if [ ! -d $LIB_SDL_DEST_DIR ]; then
  git clone --depth 1 --branch $LIB_SDL_TAG $LIB_SDL_URL $LIB_SDL_DEST_DIR || { echo "Could not clone $LIB_SDL_URL" ; exit 1; }
  cd $LIB_SDL_DEST_DIR
else
  cd $LIB_SDL_DEST_DIR
  git pull
  git checkout $LIB_SDL_TAG
fi

./autogen.sh
./configure $CONFIGURE_FLAGS
make -j3

[ -f $LIB_SDL_TARGET ] || { echo "Missing $LIB_SDL_TARGET..." ; exit 1; }

cp "$LIB_SDL_TARGET" "../../$LIB_DIR"
cd "../.."

# Build SDL_image

if [ ! -d $LIB_SDL_IMAGE_DEST_DIR ]; then
  git clone --recurse-submodules --depth 1 --branch $LIB_SDL_IMAGE_TAG $LIB_SDL_IMAGE_URL $LIB_SDL_IMAGE_DEST_DIR || { echo "Could not clone $LIB_SDL_IMAGE_URL" ; exit 1; }
  cd $LIB_SDL_IMAGE_DEST_DIR
else
  cd $LIB_SDL_IMAGE_DEST_DIR
  git pull
  git checkout $LIB_SDL_IMAGE_TAG
  git submodule update --recursive
fi

./autogen.sh
./configure $CONFIGURE_FLAGS $LIB_SDL_IMAGE_FEATURE_FLAGS
make -j3

[ -f $LIB_SDL_IMAGE_TARGET ] || { echo "Missing $LIB_SDL_IMAGE_TARGET..." ; exit 1; }

cp "$LIB_SDL_IMAGE_TARGET" "../../$LIB_DIR"
cd "../.."

# Build SDL_ttf (presumes freetype is already installed with brew)

if [ ! -d $LIB_SDL_TTF_DEST_DIR ]; then
  git clone --recurse-submodules --depth 1 --branch $LIB_SDL_TTF_TAG $LIB_SDL_TTF_URL $LIB_SDL_TTF_DEST_DIR || { echo "Could not clone $LIB_SDL_TTF_URL" ; exit 1; }
  cd $LIB_SDL_TTF_DEST_DIR
else
  cd $LIB_SDL_TTF_DEST_DIR
  git pull
  git checkout $LIB_SDL_TTF_TAG
  git submodule update --recursive
fi

./autogen.sh
./configure $CONFIGURE_FLAGS
make -j3

[ -f $LIB_SDL_TTF_TARGET ] || { echo "Missing $LIB_SDL_TTF_TARGET..." ; exit 1; }

cp "$LIB_SDL_TTF_TARGET" "../../$LIB_DIR"
cd "../.."

# Build OpenSSL

if [ ! -d $LIB_OPENSSL_DEST_DIR ]; then
  git clone --depth 1 --branch $LIB_OPENSSL_TAG $LIB_OPENSSL_URL $LIB_OPENSSL_DEST_DIR || { echo "Could not clone $LIB_OPENSSL_URL" ; exit 1; }
  cd $LIB_OPENSSL_DEST_DIR
else
  cd $LIB_OPENSSL_DEST_DIR
  git pull
  git checkout $LIB_OPENSSL_TAG
fi

./config $CONFIGURE_FLAGS
make -j4

[ -f $LIB_OPENSSL_TARGET ] || { echo "Missing $LIB_OPENSSL_TARGET..." ; exit 1; }
[ -f $LIB_CRYPTO_TARGET ] || { echo "Missing $LIB_CRYPTO_TARGET..." ; exit 1; }

cp "$LIB_OPENSSL_TARGET" "../../$LIB_DIR"
cp "$LIB_CRYPTO_TARGET" "../../$LIB_DIR"
cd "../.."

