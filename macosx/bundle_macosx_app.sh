#!/bin/bash

# script to fetch SDL and create macos .app bundle
# (c) 2021 sbzappa

EXE=
VERSION=
BUNDLE_NAME_SET=no
BUNDLE_NAME=
APP_OWNER="com.evermizer"
DEPLOYMENT_TARGET="10.12"
BUILD_THIRD_PARTY=yes

function usage() {
  echo "Usage: `basename $0` [--version=] [--bundle-name=] [--deployment-target=] [--do-not-build-thirdparty] binary"
}

if test $# -eq 0; then
  usage
  exit 1
fi

while test $# -gt 0; do
  case "$1" in
  -*=*) optarg=`echo "$1" | sed 's/[-_a-zA-Z0-9]*=//'` ;;
  *) optarg= ;;
  esac

  case $1 in
    --version=*)
      VERSION=$optarg
      ;;
    --bundle-name=*)
      BUNDLE_NAME=$optarg
      BUNDLE_NAME_SET=yes
      ;;
    --deployment-target=*)
      DEPLOYMENT_TARGET=$optarg
      ;;
    --do-not-build-thirdparty)
      BUILD_THIRD_PARTY=no
      ;;
    *)
      EXE=$1
      if test $BUNDLE_NAME_SET = no ; then
        BUNDLE_NAME=`basename $1`
      fi
      ;;
  esac
  shift
done

APP_NAME=`basename $EXE`

SRC_DIR=`dirname $0`
DST_DIR=`dirname $EXE`

ROOT_DIR="$SRC_DIR/.."
SRC_ASSETS_DIR="$ROOT_DIR/assets"
DOCS="$ROOT_DIR/LICENSE $ROOT_DIR/README.md $ROOT_DIR/CHANGELOG.md $ROOT_DIR/CREDITS.md"

APP_BUNDLE_DIR="$DST_DIR/$BUNDLE_NAME.app"
APP_BUNDLE_CONTENTS_DIR="$APP_BUNDLE_DIR/Contents"
APP_BUNDLE_MACOS_DIR="$APP_BUNDLE_CONTENTS_DIR/MacOS"
APP_BUNDLE_FRAMEWORKS_DIR="$APP_BUNDLE_CONTENTS_DIR/Frameworks"
APP_BUNDLE_RESOURCES_DIR="$APP_BUNDLE_CONTENTS_DIR/Resources"

DST_EXE=$APP_BUNDLE_MACOS_DIR/$APP_NAME

# Create app folder structure
rm -fr $APP_BUNDLE_DIR

mkdir -p $APP_BUNDLE_MACOS_DIR
mkdir -p $APP_BUNDLE_FRAMEWORKS_DIR
mkdir -p $APP_BUNDLE_RESOURCES_DIR

# Create Info.plist in app bundle
APP_BUNDLE_INFO_PLIST="$APP_BUNDLE_CONTENTS_DIR/Info.plist"

cat <<EOT >> $APP_BUNDLE_INFO_PLIST
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple Computer//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
  <key>CFBundleGetInfoString</key>
  <string>$BUNDLE_NAME</string>
  <key>CFBundleExecutable</key>
  <string>$APP_NAME</string>
  <key>CFBundleIdentifier</key>
  <string>$APP_OWNER.$APP_NAME</string>
  <key>CFBundleName</key>
  <string>$BUNDLE_NAME</string>
  <key>CFBundleShortVersionString</key>
  <string>$VERSION</string>
  <key>CFBundleInfoDictionaryVersion</key>
  <string>6.0</string>
  <key>CFBundlePackageType</key>
  <string>APPL</string>
</dict>
</plist>
EOT

# Copy binary into app bundle
cp $EXE $DST_EXE
cp -r $SRC_ASSETS_DIR $APP_BUNDLE_MACOS_DIR
cp -r $DOCS $APP_BUNDLE_MACOS_DIR

# Build third party libraries and update dynamic paths
# This won't work with libraries installed with brew.

BUILD_DIR="build"
LIB_DIR="libs"

if test $BUILD_THIRD_PARTY = yes ; then
  sh $SRC_DIR/build_thirdparty \
    --deployment-target=$DEPLOYMENT_TARGET \
    --BUILD_DIR=$BUILD_DIR \
    --LIB_DIR=$LIB_DIR
fi

LIB_SDL2="libSDL2-2.0.0.dylib"
LIB_SDL2_IMAGE="libSDL2_image-2.0.0.dylib"
LIB_SDL2_TTF="libSDL2_ttf-2.0.0.dylib"
LIB_FREETYPE="libfreetype.6.dylib"
LIB_PNG="libpng16.16.dylib"
LIB_OPENSSL="libssl.1.1.dylib"
LIB_CRYPTO="libcrypto.1.1.dylib"

cp "$SRC_DIR/$LIB_DIR/$LIB_SDL2" $APP_BUNDLE_FRAMEWORKS_DIR
cp "$SRC_DIR/$LIB_DIR/$LIB_SDL2_IMAGE" $APP_BUNDLE_FRAMEWORKS_DIR
cp "$SRC_DIR/$LIB_DIR/$LIB_SDL2_TTF" $APP_BUNDLE_FRAMEWORKS_DIR
cp "$SRC_DIR/$LIB_DIR/$LIB_FREETYPE" $APP_BUNDLE_FRAMEWORKS_DIR
cp "$SRC_DIR/$LIB_DIR/$LIB_PNG" $APP_BUNDLE_FRAMEWORKS_DIR
cp "$SRC_DIR/$LIB_DIR/$LIB_OPENSSL" $APP_BUNDLE_FRAMEWORKS_DIR
cp "$SRC_DIR/$LIB_DIR/$LIB_CRYPTO" $APP_BUNDLE_FRAMEWORKS_DIR

# Change paths on libSDL

install_name_tool -id @executable_path/../Frameworks/$LIB_SDL2 $APP_BUNDLE_FRAMEWORKS_DIR/$LIB_SDL2

# Change paths on libSDL_image

install_name_tool -id @executable_path/../Frameworks/$LIB_SDL2_IMAGE $APP_BUNDLE_FRAMEWORKS_DIR/$LIB_SDL2_IMAGE

OLD_LIB_SDL2=`otool -LX  $APP_BUNDLE_FRAMEWORKS_DIR/$LIB_SDL2_IMAGE | grep $LIB_SDL2 | awk '{print $1}'`
install_name_tool -change $OLD_LIB_SDL2 @executable_path/../Frameworks/$LIB_SDL2 $APP_BUNDLE_FRAMEWORKS_DIR/$LIB_SDL2_IMAGE

# Change paths on libSDL_ttf

install_name_tool -id @executable_path/../Frameworks/$LIB_SDL2_TTF $APP_BUNDLE_FRAMEWORKS_DIR/$LIB_SDL2_TTF

OLD_LIB_SDL2=`otool -LX  $APP_BUNDLE_FRAMEWORKS_DIR/$LIB_SDL2_TTF | grep $LIB_SDL2 | awk '{print $1}'`
OLD_LIB_FREETYPE=`otool -LX  $APP_BUNDLE_FRAMEWORKS_DIR/$LIB_SDL2_TTF | grep $LIB_FREETYPE | awk '{print $1}'`

install_name_tool -change $OLD_LIB_SDL2 @executable_path/../Frameworks/$LIB_SDL2 $APP_BUNDLE_FRAMEWORKS_DIR/$LIB_SDL2_TTF
install_name_tool -change $OLD_LIB_FREETYPE @executable_path/../Frameworks/$LIB_FREETYPE $APP_BUNDLE_FRAMEWORKS_DIR/$LIB_SDL2_TTF

# Change paths on libpng

install_name_tool -id @executable_path/../Frameworks/$LIB_PNG $APP_BUNDLE_FRAMEWORKS_DIR/$LIB_PNG

# Change paths on Freetype

install_name_tool -id @executable_path/../Frameworks/$LIB_FREETYPE $APP_BUNDLE_FRAMEWORKS_DIR/$LIB_FREETYPE

OLD_LIB_PNG=`otool -LX $APP_BUNDLE_FRAMEWORKS_DIR/$LIB_FREETYPE | grep $LIB_PNG | awk '{print $1}'`
install_name_tool -change $OLD_LIB_PNG @executable_path/../Frameworks/$LIB_PNG $APP_BUNDLE_FRAMEWORKS_DIR/$LIB_FREETYPE

# Change paths on libOpenSSL

install_name_tool -id @executable_path/../Frameworks/$LIB_OPENSSL $APP_BUNDLE_FRAMEWORKS_DIR/$LIB_OPENSSL

OLD_LIB_CRYPTO=`otool -LX $APP_BUNDLE_FRAMEWORKS_DIR/$LIB_OPENSSL | grep $LIB_CRYPTO | awk '{print $1}'`
install_name_tool -change $OLD_LIB_CRYPTO @executable_path/../Frameworks/$LIB_CRYPTO $APP_BUNDLE_FRAMEWORKS_DIR/$LIB_OPENSSL

# Change paths on libCrypto

install_name_tool -id @executable_path/../Frameworks/$LIB_CRYPTO $APP_BUNDLE_FRAMEWORKS_DIR/$LIB_CRYPTO

# Change paths on main EXE

OLD_LIB_SDL2=`otool -LX $EXE | grep $LIB_SDL2 | awk '{print $1}'`
OLD_LIB_SDL2_IMAGE=`otool -LX $EXE | grep $LIB_SDL2_IMAGE | awk '{print $1}'`
OLD_LIB_SDL2_TTF=`otool -LX $EXE | grep $LIB_SDL2_TTF | awk '{print $1}'`
OLD_LIB_OPENSSL=`otool -LX $EXE | grep $LIB_OPENSSL | awk '{print $1}'`
OLD_LIB_CRYPTO=`otool -LX $EXE | grep $LIB_CRYPTO | awk '{print $1}'`

install_name_tool -change $OLD_LIB_SDL2 @executable_path/../Frameworks/$LIB_SDL2 $DST_EXE
install_name_tool -change $OLD_LIB_SDL2_IMAGE @executable_path/../Frameworks/$LIB_SDL2_IMAGE $DST_EXE
install_name_tool -change $OLD_LIB_SDL2_TTF @executable_path/../Frameworks/$LIB_SDL2_TTF $DST_EXE
install_name_tool -change $OLD_LIB_OPENSSL @executable_path/../Frameworks/$LIB_OPENSSL $DST_EXE
install_name_tool -change $OLD_LIB_CRYPTO @executable_path/../Frameworks/$LIB_CRYPTO $DST_EXE

