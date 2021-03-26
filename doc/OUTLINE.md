# PopTracker Overview

This is a rough overview of internals and what we (have to) provide and what dependencies we have.

Check linked docs for specifics.

## Goal
- scriptable tracker and map tracker, similar to existing solutions
- open source, platform independent 
- runs in browser and on desktop
  - have a Makefile that builds WASM blob, Linux binaries, Windows exe
  - OSX support?
- auto-tracking support
  - usb2snes interface
  - more TBD
- provide an interface that is compatible to existing packs

See [TODO.md](TODO.md) for current state of things.

## WASM work-arounds
- `while(true)` does not work in WASM, so we define an interface `App` that
  - implements a mainloop on desktop
  - hooks into AnimationFrame via `emscripten_set_main_loop` in WASM

## UI requirements
- item/status "icons" inside hbox/vbox/grid
- map display: custom widget
- map "icons" drawn in mapwidget's render()
- hover -> tooltips -> floating regular widgets/containers
- have "icons" be just widgets, state/image updated from back-end events/signals
- use one TrackerView(Tracker*) widget to link up state with ui
- we may require a "texture store" to support WebGL on mobile
- layout.json => LayoutNodes => Widgets

see [UILIB.md](UILIB.md) (outdated)

NOTE: UI needs a lot of rework.


## Interface ideas
### Packs
```
class Pack abstracts away Filesystem
    ::Pack("path") loads a pack, where path can be a directory or .zip file containing a manifest.json
    ::ReadFile(std::string fn,std::string& buf) will read file `fn` of the package into buffer `buf` and return a result code (no exceptions)
    static ::ListPacks() returns a list of packs
    ::ListVariants() returns a list of available variants for the pack
    ::getVariant() returns currently active variant (or "" if none is active)
    ::setVariant("variant") sets the active variant, used when resolving paths
```

Note that std::string allows easier integration with C code than a std::stream.

We may want to use mmap or raw pointer in the future, see [Ideal ZIP](#ideal-zip).

### Lua Glue
since packs use lua to do actual stuff, we map c++ classes into lua.
See [LUAGLUE.md](LUAGLUE.md) for the lua interface code.

### Game pack interface
see [PACKS.md](PACKS.md)

We do not use the data directly, but instead load it into objects via
Class::FromJSON, which are then used for to the actual (UI) implementation.


## Back-end requirements
- LUA VM
- JSON parser
- UI/Layout abstraction (JSON -> Widgets -> Pixels)

## Base System / Dependencies
- libsdl
- libsdl_ttf
- libfreetype as dependency of sdl_ttf
- libsdl_image
- libpng as dependency of sdl_image
- zlib
- liblua
- json.hpp - https://github.com/nlohmann/json + patch to allow trailing commas

## Linking
emcc for web/wasm deployment provides some (specialized) versions of some libs:
- SDL2
- SDL2_image
- SDL2_ttf
- libpng
- zlib
- and their dependencies

for now we link the libraries dynamically on Linux and use prebuilt SDL-devel (mingw) libs on Windows,
but a more comlpex source tree + Makefile could integrate them as well

## UI-Data-Bindings
- signal Tracker::onLayoutChanged -> Ui::TrackerView::relayout
- signal Tracker::onStateChanged -> Ui::Item update
- signal Ui::Item::onClick -> ... -> Tracker::... -> Tracker::onStateChanged

## Ideal ZIP
- You would use anything (cloned repo or github-generated ZIP) during development and publish a "release ZIP" once done.
- Ideally the release ZIP would come in two versions, one to be consumed on desktop, one to be consumed by Web browsers.
- By having files aligned and uncompressed inside a ZIP, they can be accessed with mmap or void* without extraction (for a future version maybe).
- (Desktop zip like apk, Web zip like usdz)

### Desktop ZIP
- Optimized ZIP (zopfli, advzip, 7z -mx=9, ...) for fastest download/disk loading
- Optimized PNGs (zopflipng, optipng, pngcrush, ...) for fastes decompression
- Store uncompressable/precompressed files (PNG, JPG) uncompressed in ZIP
- Use zipalign to align all uncompressed files for direct consumption (mmap)

### Web ZIP
see above, but ideally you want
- **uncompressed zip** that is served over a **compressed connection**
- zipalign **all** files (since all are uncompressed)
- serve .br (brotli) or .gz (gzip) as indicated by the browser
- both will compress the whole stream as one file, so duplicates in multiple files will have better compression than would be possible in a .zip
- brotli has better compression than deflate (gzip,zip)
- use .htaccess with mod_rewrite to serve precompressed files for optimum size and performance
- after downloading, all files will be available uncompressed in the browser cache, aligned for direct consumption

