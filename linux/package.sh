#!/usr/bin/env sh

# Script to package a portable Linux build.
# The build will be reconfigured with `--prefix / --bindir . --datadir .`
# before install to match the expected file layout for a portable build.

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
  meson setup --reconfigure --prefix / --bindir . --datadir . "$build_dir" "$MESON_SOURCE_ROOT"
  arch=$(meson introspect --machines "$build_dir" | jq -r '.host.cpu_family')
  if [ -z "$arch" ]; then
    arch=$(grep 'Host machine cpu family:' "$build_dir/meson-logs/meson-setup.txt" | rev | cut -F1 | rev)
    if [ -z "$arch" ]; then
      arch=$(uname -m)
      echo "Couldn't detect arch! Guessing $arch." >&2
    fi
  fi
  vs=$(meson introspect --projectinfo "$build_dir" | jq -r '.version' | tr -s '.' '-')
  distro=$(lsb_release -si | tr -s ' ' '-' | tr '[:upper:]' '[:lower:]')
  distro_vs=$(lsb_release -sr | tr -s '.' '-' | tr '[:upper:]' '[:lower:]')
  tar_name="${app}_${vs}_${distro}-${distro_vs}-${arch}.tar.xz"
  temp_dir=$(mktemp -d -t "$app.tar.xz.XXXXXXXXXX")
  temp_dest_dir="$temp_dir/$app"
  temp_tar_file="$temp_dir/$tar_name"
  mkdir -p "$temp_dest_dir"
  meson install -C "$build_dir" "$@" --destdir "$temp_dest_dir"
  echo "Packaging $temp_dest_dir into $tar_name"
  [ -f "$temp_dest_dir/$app.exe" ] && echo "WARNING: Looks like a Windows build!" >&2
	(
	  cd "$temp_dir"
	  tar -cJf "$tar_name" "$app"
	)
  mkdir -p "$dist_dir"
  mv "$temp_tar_file" "$dist_dir"
  rm -r "$temp_dir"
)
