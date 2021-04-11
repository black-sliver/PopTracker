#!/bin/bash

function usage() {
  echo "Usage: `basename $0` binary"
}

if [ -z "$1" ]
then
  usage
  exit 0
fi

SRC_EXE=$1
VERSION="0.1"

APP_NAME=`basename $SRC_EXE`

SRC_DIR=`dirname $0`
DST_DIR=`dirname $1`

SRC_ASSETS_DIR="$SRC_DIR/assets"

APP_BUNDLE_DIR="$SRC_EXE.app"
APP_BUNDLE_CONTENTS_DIR="$SRC_EXE.app/Contents"
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
  <string>$APP_NAME</string>
  <key>CFBundleExecutable</key>
  <string>$APP_NAME</string>
  <key>CFBundleIdentifier</key>
  <string>com.blacksliver.$APP_NAME</string>
  <key>CFBundleName</key>
  <string>$APP_NAME</string>
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
cp $SRC_EXE $DST_EXE
cp -r $SRC_ASSETS_DIR $APP_BUNDLE_MACOS_DIR

# Update dynamic paths
# This won't work with libraries installed with brew as 

SRC_LIB_SDL2=`otool -LX $SRC_EXE | grep "libSDL2-2.0.0" | awk '{print $1}'`
SRC_LIB_SDL2_IMAGE=`otool -LX $SRC_EXE | grep "libSDL2_image-2.0.0" | awk '{print $1}'`
SRC_LIB_SDL2_TTF=`otool -LX $SRC_EXE | grep "libSDL2_ttf-2.0.0" | awk '{print $1}'`

SRC_LIB_FREETYPE=`otool -LX $SRC_LIB_SDL2_TTF | grep "libfreetype.6" | awk '{print $1}'`
SRC_LIB_PNG=`otool -LX $SRC_LIB_FREETYPE | grep "libpng16.16" | awk '{print $1}'`

LIB_SDL2=`basename $SRC_LIB_SDL2`
LIB_SDL2_IMAGE=`basename $SRC_LIB_SDL2_IMAGE`
LIB_SDL2_TTF=`basename $SRC_LIB_SDL2_TTF`
LIB_FREETYPE=`basename $SRC_LIB_FREETYPE`
LIB_PNG=`basename $SRC_LIB_PNG`

DST_LIB_SDL2=$APP_BUNDLE_FRAMEWORKS_DIR/$LIB_SDL2
DST_LIB_SDL2_IMAGE=$APP_BUNDLE_FRAMEWORKS_DIR/$LIB_SDL2_IMAGE
DST_LIB_SDL2_TTF=$APP_BUNDLE_FRAMEWORKS_DIR/$LIB_SDL2_TTF
DST_LIB_FREETYPE=$APP_BUNDLE_FRAMEWORKS_DIR/$LIB_FREETYPE
DST_LIB_PNG=$APP_BUNDLE_FRAMEWORKS_DIR/$LIB_PNG

cp $SRC_LIB_SDL2 $DST_LIB_SDL2
cp $SRC_LIB_SDL2_IMAGE $DST_LIB_SDL2_IMAGE
cp $SRC_LIB_SDL2_TTF $DST_LIB_SDL2_TTF
cp $SRC_LIB_FREETYPE $DST_LIB_FREETYPE
cp $SRC_LIB_PNG $DST_LIB_PNG

install_name_tool -id @executable_path/../Frameworks/$LIB_SDL2 $DST_LIB_SDL2

install_name_tool -id @executable_path/../Frameworks/$LIB_SDL2_TTF $DST_LIB_SDL2_TTF
install_name_tool -change $SRC_LIB_SDL2 @executable_path/../Frameworks/$LIB_SDL2 $DST_LIB_SDL2_TTF

install_name_tool -id @executable_path/../Frameworks/$LIB_SDL2_IMAGE $DST_LIB_SDL2_IMAGE
install_name_tool -change $SRC_LIB_SDL2 @executable_path/../Frameworks/$LIB_SDL2 $DST_LIB_SDL2_IMAGE

install_name_tool -id @executable_path/../Frameworks/$LIB_FREETYPE $DST_LIB_FREETYPE
install_name_tool -change $SRC_LIB_PNG @executable_path/../Frameworks/$LIB_PNG $DST_LIB_FREETYPE

install_name_tool -id @executable_path/../Frameworks/$LIB_PNG $DST_LIB_PNG

install_name_tool -change $SRC_LIB_SDL2 @executable_path/../Frameworks/$LIB_SDL2 $DST_EXE
install_name_tool -change $SRC_LIB_SDL2_TTF @executable_path/../Frameworks/$LIB_SDL2_TTF $DST_EXE
install_name_tool -change $SRC_LIB_SDL2_IMAGE @executable_path/../Frameworks/$LIB_SDL2_IMAGE $DST_EXE

