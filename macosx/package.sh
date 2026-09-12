#!/usr/bin/env sh

# Script to package a macOS app bundle folder into a zip file.
# The script will first run bundle*.sh to create the app bundle.

set -e

app="poptracker"

if [ -z "$MESON_SOURCE_ROOT" ]; then
  # shellcheck disable=SC3054
  if [ -z "${BASH_SOURCE[0]}" ]; then
    MESON_SOURCE_ROOT="."  # fall back to assume running from source root
  else
    MESON_SOURCE_ROOT="$( dirname -- "${BASH_SOURCE[0]}"; )/..";
  fi
fi

if [ -z "$1" ]; then
  echo "arg required: build-dir" >&2
  exit 1
fi

dist_dir="$MESON_SOURCE_ROOT/dist"
build_dir="$1"
shift

(
  meson compile -C "$build_dir" appbundle
  arch=$(meson introspect --machines "$build_dir" | jq -r '.host.cpu_family')
  if [ -z "$arch" ]; then
    arch=$(grep 'Host machine cpu family:' "$build_dir/meson-logs/meson-setup.txt" | rev | cut -F1 | rev)
    if [ -z "$arch" ]; then
      arch=$(uname -m)
      echo "Couldn't detect arch! Guessing $arch." >&2
    fi
  fi
  if [ "$arch" = "aarch64" ]; then
    platform="macos_arm64"  # match the uname -m output
  else
    platform="macos_$arch"
  fi
  vs=$(meson introspect --projectinfo "$build_dir" | jq -r '.version' | tr -s '.' '-')
  zip_name="${app}_${vs}_${platform}.zip"
  echo "Packaging $build_dir/$app.app into $dist_dir/$zip_name"
  mkdir -p "$dist_dir"
  # Use ditto (instead of zip/7z) to keep mac-specific metadata like codesign.
  ditto -c -k --keepParent --sequesterRsrc --zlibCompressionLevel 9 "$build_dir/$app" "$dist_dir/$zip_name"
)
