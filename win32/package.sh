#!/usr/bin/env sh

# Script to package a Windows build.
# The build will be reconfigured with `--prefix <temp> --bindir . --datadir .`
# before install to match the expected file layout for Windows.

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
  # On Windows, setup --prefix '/' + install --destdir ... does not work, so we supply the full path here:
  temp_dir=$(mktemp -d -t "$app.zip.XXXXXXXXXX")
  temp_dest_dir="$temp_dir/$app"
  meson setup --reconfigure --prefix "$temp_dest_dir" --bindir . --datadir . "$build_dir" "$MESON_SOURCE_ROOT"
  arch=$(meson introspect --machines "$build_dir" | jq -r '.host.cpu_family')
  if [ -z "$arch" ]; then
    arch=$(grep 'Host machine cpu family:' "$build_dir/meson-logs/meson-setup.txt" | rev | cut -F1 | rev)
    if [ -z "$arch" ]; then
      arch=$(uname -m)
      echo "Couldn't detect arch! Guessing $arch." >&2
    fi
  fi
  if [ "$arch" = "x86_64" ]; then
    platform="win64"
  elif [ "$arch" = "x86" ]; then
    platform="win32"
  elif [ "$arch" = "arm" ]; then
    platform="winarm32"
  elif [ "$arch" = "aarch64" ]; then
    platform="winarm64"
  else
    platform="win"
  fi
  vs=$(meson introspect --projectinfo "$build_dir" | jq -r '.version' | tr -s '.' '-')
  zip_name="${app}_${vs}_${platform}.zip"
  temp_zip_file="$temp_dir/$zip_name"
  updater="$build_dir/PopUpdater.exe"
  mkdir -p "$temp_dest_dir"
  meson install -C "$build_dir" "$@"
  cp -a "$updater" "$temp_dest_dir" || echo "WARNING: PopUpdater not found!" >&2
  echo "Packaging $temp_dest_dir into $zip_name"
  [ -f "$temp_dest_dir/$app.exe" ] || echo "WARNING: Doesn't look like a Windows build!" >&2
	(
	  cd "$temp_dir"
	  if type 7z >/dev/null 2>&1; then
	    7z a -mx=9 "$zip_name" "$app"
	  else
      zip -9 -r "$zip_name" "$app"
    fi
    if type advzip >/dev/null 2>&1; then
      advzip --recompress -4 "$zip_name"
    fi
	)
  mkdir -p "$dist_dir"
  mv "$temp_zip_file" "$dist_dir"
  rm -r "$temp_dir"
)
