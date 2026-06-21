#!/bin/bash
# Combine two single-architecture macOS .app bundles into one universal2 bundle.
#
# Usage: make_universal.sh <app_a> <app_b> <out_app>
#
# The two inputs must be the same build for different architectures (identical
# layout). Every Mach-O file is lipo-merged; everything else (data files,
# symlinks, Info.plist, ...) is taken verbatim from the first bundle, which is
# cloned with ditto to preserve permissions, symlinks and metadata.
#
# The result is unsigned (lipo invalidates signatures) -- re-sign the output
# bundle afterwards (e.g. `codesign --force --deep --sign - <out_app>`).
set -euo pipefail

if [ "$#" -ne 3 ]; then
    echo "usage: $0 <app_a> <app_b> <out_app>" >&2
    exit 2
fi

APP_A="$1"
APP_B="$2"
OUT="$3"

rm -rf "$OUT"
ditto "$APP_A" "$OUT"   # faithful clone: perms, symlinks, xattrs

# lipo every Mach-O file that exists in both bundles
find "$APP_A" -type f -print0 | while IFS= read -r -d '' f; do
    rel="${f#"$APP_A"/}"
    if file -b "$f" | grep -q 'Mach-O' && [ -f "$APP_B/$rel" ]; then
        lipo -create "$APP_A/$rel" "$APP_B/$rel" -output "$OUT/$rel"
    fi
done

echo "Universal bundle written to: $OUT"
lipo -info "$OUT/Contents/MacOS/$(basename "${OUT%.app}")" 2>/dev/null || true
