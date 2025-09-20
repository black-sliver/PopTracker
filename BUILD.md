# PopTracker build instructions

We only use `make` at the moment. Feel free to create your own project/build setup, see [Why not XYZ?!](#why-not-xyz).

`make native` builds a native binary\
`make cross` cross-compiles a windows binary on *nix\
`make wasm` builds a wasm blob, this may be broken and does not fully work yet\
`make CONF=DIST ...` builds a zip/tar.xz in `dist/` that includes all files required to run it

Unless we decide to include source to build all dependencies, they'll have to be installed system-wide or copied to
win32-lib/lib. Static libs depend on your toolchain, so they are not included.

- Lua is built from a submodule
- Header-only libraries are included
- For other dependencies:
    - Linux: use your package manager
    - Windows: use MSYS2
    - macOS: use brew

See individual build instructions below for dependencies.

## Runtime dependencies

Windows builds are static, embedding all dependencies.

On macOS, a dev build depends on the libs installed through brew. A release build will bundle all dependencies.
See [Build on macOS](#build-on-macos) for details.

On Linux, builds are dynamically linked by default and all linked libraries are required.
In addition, a dialog provider (`zenity`, `kdialog`, `matedialog`, `qarma` or `xdialog`) and `which` are required.

Assets will have to be in one of the search paths. Read below.

## Getting the source

Run `git clone --recurse-submodules https://github.com/black-sliver/PopTracker.git`
or download the latest "full-source.tar.xz" from [Releases](https://github.com/black-sliver/PopTracker/releases).

If you forgot the `--recurse-submodules`, use `git submodule update --init --recursive`.
After pulling, if submodules changed, use `git submodule update --recursive`.

## Build on Arch Linux

### Native
- `pacman -S base-devel sdl2 sdl2_image sdl2_ttf openssl # install dependencies`
- run `make native CONF=RELEASE` to generate `./build/<platform>/poptracker` binary
- run binary with working directory set to the source directory, copy assets + binary into a single folder
  or copy assets to `~/PopTracker/assets`

### Cross Compile
- `pacman -S mingw-w64-gcc # install cross compile toolchain`
- install mingw-w64-{sdl2,sdl2_image,sdl2_ttf,openssl} from AUR # install dependencies
- run `make cross CONF=RELEASE`

### WASM
- `pacman -S emscripten # install emscripten`
- if /usr/lib/emscripten/node_modules is missing or empty, you need to fix that
    - latest arch package includes node_modules, so update should fix it
- run `make wasm CONF=RELEASE`

### Build on Ubuntu / Debian

### Native
- `sudo apt install build-essential libsdl2-dev libsdl2-image-dev libsdl2-ttf-dev libssl-dev # install dependencies`
- if you use debian older than bookworm or Ubuntu older than 22.04,
  you will have to install libsdl 2.0.18 or newer from source
- run `make native CONF=RELEASE` to generate `./build/<platform>/poptracker` binary
- run binary with working directory set to the source directory, copy assets + binary into a single folder
  or copy assets to `~/PopTracker/assets`

## Build on NixOS

### Native
- `nix-shell`
- run `make native CONF=RELEASE` to generate `./build/<platform>/poptracker` binary
- run binary with working directory set to the source directory, copy assets + binary into a single folder
  or copy assets to `~/PopTracker/assets`

## Build on Windows

### GCC / MSYS2
- install MSYS2 from https://www.msys2.org/
- use the MINGW64 terminal to install dependencies and building
- install dependencies:

  ```
  pacman -S base-devel coreutils make mingw-w64-x86_64-toolchain mingw64/mingw-w64-x86_64-SDL2 \
  mingw64/mingw-w64-x86_64-SDL2_image mingw64/mingw-w64-x86_64-SDL2_ttf mingw64/mingw-w64-x86_64-freetype \
  mingw64/mingw-w64-x86_64-openssl p7zip
  ```

  or see [GitHub workflow](https://github.com/black-sliver/PopTracker/blob/master/.github/workflows/binaries.yaml)
- run `make CONF=RELEASE`
- run exe with working directory set to the source directory, copy assets + exe into a single folder
  or copy assets to `C:\Users\<user>\PopTracker\assets`

If the Windows build is failing, MSYS probably changed libraries. Let us know on Discord or create an issue on GitHub.

The build found in Releases is done with a customized sdl2, so the builds differ from MSYS ones.

## Build on macOS

- run `brew install coreutils SDL2 sdl2_ttf sdl2_image openssl@3.0`
- run `make CONF=RELEASE`

The build will link against brew libraries.

If you run `make CONF=DIST`, this will build non-brew versions of the libraries
and replace the references in the resulting app bundle.\
Dependencies to build the bundle: install git and `brew install automake libtool autoconf`

## Why not XYZ?!

The release builds for Windows and macOS are very custom since we do not want to force msys or brew on anyone and use a
gnu toolchain for development.

* Meson's submodules system would be great, but some of them are very outdated or incomplete and maintaining the recipes
  for all dependencies is too much work.
* CMake did fail when trying to create the static windows build, at which point we'd need to modify and maintain CMake
  files for subprojects.
* Autotools could probably be used to strip down the Makefile, feel free to provide an example.
* An additional VS solution could be maintained by someone, feel free to fork, PR and document.
* An additional XCode project could be maintained by someone, feel free to fork, PR and document.
* Check [CONTRIBUTING.md](CONTRIBUTING.md#compiler-configuration) to see why we ignore your CFLAGS.
