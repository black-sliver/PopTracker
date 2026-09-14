#!/bin/bash
#
# Helper that loads exclusions from `/.scan-build-ignore`.
# Use with Meson as `SCANBUILD=../contrib/scan-build-wrapper/scan-build.sh ninja -C build scan-build`.
# Use `SCANBUILD_EXE=...` to use a specific scan-build executable, otherwise auto-detect.
#

set -e

resolve_realpath () {
  # resolve absolute path from path $1 relative to project root $2 (preferred)
  realpath "$2/$1"
}

resolve_relpath () {
  # path $1 relative to project root magically works for --exclude
  echo "$1"
}

if type realpath >/dev/null 2>/dev/null; then
  resolve=resolve_realpath
else
  resolve=resolve_relpath
fi

if [ -n "$SCANBUILD_EXE" ]; then
  scan_build="$SCANBUILD_EXE"
else
  # find latest scan-build (assume numbered versions are newer than unnumbered)
  paths=''
  for path in $(echo "$PATH" | tr ':' '\n'); do
    if [ -d "$path" ]; then
      paths=$(printf "%s %q" "$paths" "$path")
    fi
  done
  echo "Searching for scan-build in $paths ..."  # debug
  if [ -n "$paths" ]; then
    scan_build=$(
      # shellcheck disable=SC2086
      find $paths -maxdepth 1 -regex '.*/scan-build-?[0-9]*' -executable -exec basename {} \; \
        | sort -r | head -n1
    )
  fi
  # fall back to just "scan-build" for file not found message
  if [ -z "$scan_build" ]; then
    scan_build="scan-build"
  fi
fi

if [ -z "${BASH_SOURCE[0]}" ]; then
  proj_dir=".."  # assume running from /build/
else
  proj_dir="$( dirname -- "${BASH_SOURCE[0]}"; )/../..";
fi

if [ -f "$proj_dir/.scan-build-ignore" ]; then
  # if exclude file exists
  if [ "$1" == "--exclude" ]; then
    # remove default exclude (subprojects)
    shift
    shift
  fi

  while read -r line; do
    if [ -n "$line" ]; then
      # ignore lines starting with '#'
      if [ "${line#\#}"x = "${line}x" ]; then
        exclusion=$("$resolve" "$line" "$proj_dir")
        # modify $@ in-place for simplicity
        set -- "--exclude" "$exclusion" "$@"
      fi
    fi
  done < "$proj_dir/.scan-build-ignore"
fi

"$scan_build" \
  --status-bugs \
  "$@"
