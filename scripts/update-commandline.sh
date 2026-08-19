#!/usr/bin/env sh

# Script to update doc/commandline.txt from built exe.

set -e

EXE_NAME="poptracker"

if [ -z "$MESON_SOURCE_ROOT" ]; then
  # shellcheck disable=SC3054
  if [ -z "${BASH_SOURCE[0]}" ]; then
    MESON_SOURCE_ROOT="."  # fall back to assume running from source root
  else
    MESON_SOURCE_ROOT="$( dirname -- "${BASH_SOURCE[0]}"; )/..";
  fi
fi

if [ -z "$1" ]; then
  EXE="$MESON_BUILD_ROOT/$EXE_NAME"
else
  EXE="$1"
fi

"$EXE" --help | sed "s@$EXE@$EXE_NAME@" > "$MESON_SOURCE_ROOT/doc/commandline.txt"
