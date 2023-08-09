# PopTracker Linux Utils

## AppImage

This folder contains an AppImageBuilder file.

To build the AppImage, run `appimage-builder --recipe linux/AppImageBuilder.yml`.
Can't use `AppImageCrafters/build-appimage` because the docker image is too old.
To set the proper version, run `sed -i "s/version: latest/version: $VERSION/g" linux/AppImageBuilder.yml`.

### TODO

A future version may build a static binary and/or use a customized libsdl to reduce the download size.
We may want to set version or build the AppImage from make in the future.
