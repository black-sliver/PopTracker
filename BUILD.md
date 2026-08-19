# PopTracker build instructions

We use the Meson build system and provide some `make` targets for back compatibility for now.
Consider updating your workflow to use Meson directly if you still use `make`.

Unless we decide to include source to build all dependencies, they'll have to be installed system-wide or configured for
your toolchain. Static libs depend on your toolchain, so they are not included.

- Lua is built from a git submodule
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
- run `meson setup build && meson compile -C build` to generate `./build/poptracker` binary
- or run the deprecated `make native CONF=RELEASE` to generate `./build/<platform>/poptracker` binary
- run binary with working directory set to the source directory, copy assets + binary into a single folder
  or copy assets to `~/PopTracker/assets`

### Cross Compile
> [!CAUTION]  
> The easiest way to get the dependencies is from AUR, but beware of potential risks.

- `pacman -S mingw-w64-gcc meson # install cross compile toolchain and build system`
- install mingw-w64-{sdl2,sdl2_image,sdl2_ttf,openssl,pkg-config} from AUR # install dependencies
- run `meson setup --cross-file win32/x86_64-w64-mingw32.ini build/win64 && meson compile -C build/win64`
- or run the deprecated `make cross CONF=RELEASE`

### WASM
> [!IMPORTANT]  
> This is outdated and currently unsupported.

- `pacman -S emscripten # install emscripten`
- if /usr/lib/emscripten/node_modules is missing or empty, you need to fix that
    - latest arch package includes node_modules, so update should fix it
- run `make wasm CONF=RELEASE`

### Build on Ubuntu / Debian

### Native
- `sudo apt install build-essential libsdl2-dev libsdl2-image-dev libsdl2-ttf-dev libssl-dev # install dependencies`
- if you use debian older than bookworm or Ubuntu older than 22.04,
  you will have to install libsdl 2.0.18 or newer from source
- run `meson setup build && meson compile -C build` to generate `./build/poptracker` binary
- or run the deprecated `make native CONF=RELEASE` to generate `./build/<platform>/poptracker` binary
- run binary with working directory set to the source directory, copy assets + binary into a single folder
  or copy assets to `~/PopTracker/assets`

## Build on NixOS

### Native
- `nix-shell`
- run `AR=gcc-ar meson setup build && meson compile -C build` to generate `./build/poptracker` binary
- run binary with working directory set to the source directory, copy assets + binary into a single folder
  or copy assets to `~/PopTracker/assets`

> [!IMPORTANT]  
> You will get linker errors if `ar` can't find the LTO plugin. Use `gcc-ar` by setting the `AR` environment variable.

## Build on Windows

### GCC / MSYS2
- install MSYS2 from https://www.msys2.org/
- use the MINGW64 terminal to install dependencies and building
- install dependencies:

  ```
  pacman -S base-devel coreutils make mingw-w64-x86_64-toolchain mingw64/mingw-w64-x86_64-SDL2 \
  mingw64/mingw-w64-x86_64-SDL2_image mingw64/mingw-w64-x86_64-SDL2_ttf mingw64/mingw-w64-x86_64-freetype \
  mingw64/mingw-w64-x86_64-openssl p7zip meson
  ```

  or see [GitHub workflow](https://github.com/black-sliver/PopTracker/blob/master/.github/workflows/binaries.yaml)
- run `meson setup build && meson compile -C build` to generate `./build/poptracker` binary
- or run the deprecated `make CONF=RELEASE`
- run exe with working directory set to the source directory, copy assets + exe into a single folder
  or copy assets to `C:\Users\<user>\PopTracker\assets`

If the Windows build is failing, MSYS probably changed libraries. Let us know on Discord or create an issue on GitHub.

The build found in Releases is done with a customized sdl2, so the builds differ from MSYS ones.

## Build on macOS

- run `brew install coreutils SDL2 sdl2_ttf sdl2_image openssl@3.0 meson`
- run `meson setup build && meson compile -C build`
- or run the deprecated `make CONF=RELEASE`

The build will link against brew libraries.

> [!IMPORTANT]  
> The section below is outdated and needs an update for Meson.

If you run `make CONF=DIST`, this will build non-brew versions of the libraries
and replace the references in the resulting app bundle.\
Dependencies to build the bundle: install git and `brew install automake libtool autoconf`

To run tests, `brew install googletest` and then `make test`.

## Debug Builds

For debugging, the recommended config is  
`meson setup --reconfigure --buildtype debugoptimized -Doptimization=g build`

## Why not XYZ?!

The release builds for Windows and macOS are very custom since we do not want to force msys or brew on anyone and use a
gnu toolchain for development. Meson seems to be a good fit besides needing a bunch of extra scripts.

* CMake is a complete dumpster fire in general.
* An additional VS solution could be maintained by someone, feel free to fork, PR and document.
* An additional XCode project could be maintained by someone, feel free to fork, PR and document.
* Check [CONTRIBUTING.md](CONTRIBUTING.md#compiler-configuration) to see why we ignore your CFLAGS.
