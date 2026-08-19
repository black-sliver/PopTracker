#!/usr/bin/env sh

# Helper to run PopTracker with the correct working directory from `meson compile`.
# Meson's run_target does not support setting the working directory and
# running PopTracker from the build folder requires working directory being set to source root.
#
# For IDE, configure the run options (i.e. working dir) for the main_exe (poptracker) instead.


set -e

if [ -z "$MESON_SOURCE_ROOT" ]; then
  # shellcheck disable=SC3054
  if [ -z "${BASH_SOURCE[0]}" ]; then
    MESON_SOURCE_ROOT="."  # fall back to assume running from source root
  else
    MESON_SOURCE_ROOT="$( dirname -- "${BASH_SOURCE[0]}"; )/..";
  fi
fi

if [ -z "$1" ]; then
  echo "arg required: path to executable" >&2
  exit 1
fi

EXE="$1"
shift

(
  cd "$MESON_SOURCE_ROOT"
  "$EXE" "$@"
)
