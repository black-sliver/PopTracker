# PopTracker Overview

This is a rough overview of internals and what we want to provide.

Check linked docs for specifics.

## Goal
- scriptable tracker and map tracker, similar to existing solutions
- open source, platform independent 
- runs in browser and on desktop
  - have a Makefile that builds WASM blob, Linux binaries, Windows exe and macOS app bundle
- auto-tracking support
  - see [AUTOTRACKING.md] for currently implemented things
  - more TBD
- provide an interface that is compatible to existing packs
- maybe add nicer interfaces on top or in parallel

See [TODO.md](TODO.md) for current state of things.

## WASM work-arounds

## UI requirements
see [UILIB.md](UILIB.md)

## Interface ideas
std::string allows easier integration with C code than a std::stream.

We may want to use mmap or raw pointer in the future, see [Ideal ZIP](#ideal-zip).

### Lua Glue
Since packs use Lua to do actual stuff, we map C++ classes into Lua.
See [luaglue/README.md](https://github.com/black-sliver/luaglue/blob/main/README.md) for the lua interface code.

### Game pack interface
see [PACKS.md](PACKS.md)

We do not use the data directly, but instead load it into objects via
Class::FromJSON, which are then used for to the actual (UI) implementation.

## WASM support
At some point PopTracker should be able to run inside a web-browser using
[WASM](https://en.wikipedia.org/wiki/WebAssembly).
Most or all dependencies should work with emcc, but UI/window handling is not done yet nor is a virtual file system.

- `while(true)` does not work in WASM, so we define an interface `App` that
  - implements a mainloop on desktop
  - hooks into AnimationFrame via `emscripten_set_main_loop` in WASM

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
- Optimized PNGs (zopflipng, optipng, pngcrush, ...) for fastest decompression
- Store incompressible/precompressed files (PNG, JPG) uncompressed in ZIP
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

