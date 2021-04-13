#!/bin/bash

EXE=
VERSION=
BUNDLE_NAME_SET=no
BUNDLE_NAME=

function usage() {
  echo "Usage: `basename $0` [--version=] [--bundle-name=] binary"
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

SRC_ASSETS_DIR="$SRC_DIR/../assets"

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
  <string>com.blacksliver.$APP_NAME</string>
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

# Update dynamic paths
# This won't work with libraries installed with brew.

# Temporary solution to retrieve the necessary libraries to produce a app bundle.
# A more permanent solution would be to download, compile, and link against the sources directly.
REPO_URL="https://github.com/sbzappa/prebuilt-sdl.git"
PREBUILT_DIR="$SRC_DIR/prebuilt-sdl"

[ -d $PREBUILT_DIR ] || git clone $REPO_URL $PREBUILT_DIR
[ -d $PREBUILT_DIR ] || exit 1

LIB_SDL2="libSDL2-2.0.0.dylib"
LIB_SDL2_IMAGE="libSDL2_image-2.0.0.dylib"
LIB_SDL2_TTF="libSDL2_ttf-2.0.0.dylib"
LIB_FREETYPE="libfreetype.6.dylib"
LIB_PNG="libpng16.16.dylib"

cp "$PREBUILT_DIR/$LIB_SDL2" $APP_BUNDLE_FRAMEWORKS_DIR
cp "$PREBUILT_DIR/$LIB_SDL2_IMAGE" $APP_BUNDLE_FRAMEWORKS_DIR
cp "$PREBUILT_DIR/$LIB_SDL2_TTF" $APP_BUNDLE_FRAMEWORKS_DIR
cp "$PREBUILT_DIR/$LIB_FREETYPE" $APP_BUNDLE_FRAMEWORKS_DIR
cp "$PREBUILT_DIR/$LIB_PNG" $APP_BUNDLE_FRAMEWORKS_DIR

OLD_LIB_SDL2=`otool -LX $EXE | grep $LIB_SDL2 | awk '{print $1}'`
OLD_LIB_SDL2_IMAGE=`otool -LX $EXE | grep $LIB_SDL2_IMAGE | awk '{print $1}'`
OLD_LIB_SDL2_TTF=`otool -LX $EXE | grep $LIB_SDL2_TTF | awk '{print $1}'`

install_name_tool -change $OLD_LIB_SDL2 @executable_path/../Frameworks/$LIB_SDL2 $DST_EXE
install_name_tool -change $OLD_LIB_SDL2_IMAGE @executable_path/../Frameworks/$LIB_SDL2_IMAGE $DST_EXE
install_name_tool -change $OLD_LIB_SDL2_TTF @executable_path/../Frameworks/$LIB_SDL2_TTF $DST_EXE

