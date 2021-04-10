# PopTracker build instructions
We only use make at the moment. Feel free to create your own project/build setup.

`make native` builds a native binary on *nix, not implemented on windows yet
`make cross` cross-compiles a windows binary on *nix
`make wasm` builds a wasm blob

Unless we decide to include source to build all dependencies, they'll have to be
installed system-wide or copied to win32-lib/lib. Static libs depend on your
toolchain, so they are not included.

Lua is built from a submodule.

**TODO**: document other dependencies

## Getting the source
Run `git clone --recurse-submodules https://github.com/black-sliver/PopTracker.git`
or download the zip and zips of linked submodules and extract them.

## Arch Linux
### Native
- `pacman -S base-devel sdl2 sdl2_image sdl2_ttf # install dependencies`
- run `make native CONF=RELEASE` to generate `./build/<platform>/poptracker` binary

### Cross Compile
- `pacman -S mingw-w64-gcc # install cross compile toolchain`
- ...
- run `make cross CONF=RELEASE`

### WASM
- `pacman -S emscripten # install emscripten`
- if /usr/lib/emscripten/node_modules is missing or empty, you need to fix that
    - latest arch package includes node_modules, so update should fix it
- run `make wasm CONF=RELEASE`

## Ubuntu
### Native
- `sudo apt install build-essential libsdl2-dev libsdl2-image-dev libsdl2-ttf-dev # install dependencies`
- run `make native CONF=RELEASE` to generate `./build/<platform>/poptracker` binary
