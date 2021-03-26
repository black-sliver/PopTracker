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

## Arch Linux
### Native
- `pacman -S base-devel sdl2 sdl2_image sdl2_ttf # install dependencies`
- run `make native`

### Cross Compile
- `pacman -S mingw-w64-gcc # install cross compile toolchain`
- ...
- run `make cross`

### WASM
- `pacman -S emscripten # install emscripten`
- if /usr/lib/emscripten/node_modules is missing or empty, you need to fix that
    - latest arch package includes node_modules, so update should fix it
- run `make wasm`

