#!/usr/bin/env sh

# Script to download cacert.pem from curl.se in the nicest way possible:
# * only download .pem if sha256 changed
# * only download sha256 if etag changed
# Falls back to wget if curl is not available.
# run with ./scripts/update-cacert.sh or meson compile -C build update-cacert

set -e

CACERT_URL="https://curl.se/ca/cacert.pem"
SHA256_URL="https://curl.se/ca/cacert.pem.sha256"

if [ -z "$MESON_SOURCE_ROOT" ]; then
  # shellcheck disable=SC3054
  if [ -z "${BASH_SOURCE[0]}" ]; then
    MESON_SOURCE_ROOT="."  # fall back to assume running from source root
  else
    MESON_SOURCE_ROOT="$( dirname -- "${BASH_SOURCE[0]}"; )/..";
  fi
fi

(
  cd "$MESON_SOURCE_ROOT"

  CACERT_FILE_NAME="cacert.pem"  # inside assets folder
  SHA256_FILE_NAME="$CACERT_FILE_NAME.sha256"  # inside source root
  if [ -f ".cacert.sha256.etag" ]; then
    SHA256_ETAG_FILE_NAME=".cacert.sha256.etag"  # inside source root; old name
    # TODO: mv old name to new name once the new script has been proven to work
  else
    SHA256_ETAG_FILE_NAME=".$SHA256_FILE_NAME.etag"  # inside source root; new name
  fi

  CURL=$(which curl)

  (
    cd assets && \
      if [ -x "$CURL" ]; then \
        curl --etag-compare "../$SHA256_ETAG_FILE_NAME" \
          --etag-save "../$SHA256_ETAG_FILE_NAME" \
          -o "$SHA256_FILE_NAME" "$SHA256_URL" && \
          if [ -f "$SHA256_FILE_NAME" ]; then \
            sha256sum -c "$SHA256_FILE_NAME" || ( \
              curl -o "$CACERT_FILE_NAME" "$CACERT_URL" && \
              sha256sum -c "$SHA256_FILE_NAME" \
            ) \
          fi \
      else \
        wget -O "$SHA256_FILE_NAME" "$SHA256_URL" && \
          sha256sum -c "$SHA256_FILE_NAME" || ( \
            wget -O "$CACERT_FILE_NAME" "$CACERT_URL" && \
              sha256sum -c "$SHA256_FILE_NAME" \
          ) \
      fi \
  )
  rm -f "assets/$SHA256_FILE_NAME"
)
