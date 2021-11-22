# PopTracker TODO

Check [OUTLINE.md](OUTLINE.md) to get an overview of everything.

## Current Plans
see [github projects](https://github.com/black-sliver/PopTracker/projects)

## Help wanted
- OSX binaries (.dmg, building SDL from source)
- Custom application icon
- Fancier GUI
  - Better widgets ("real" buttons)
  - Nicer icons (I'm not an artist)

## General Stuff
- isReachable optimizations:
    - do not call into Lua from updateLocationState(): prebuild cache
    - invalidate only parts of _reachableCache when toggling items?
    - update location state on next frame instead of immediately (start of auto-tracking)
    - try to determine if location state needs update?
- show version somewhere in the app
- MemoryWatch: only run callback if all bytes of a watch have been read to avoid potential races (this has to add race-free bool to SNES::readBlock)
- auto-tracking for other systems than SNES
- pins
- notes
- background color of groups/titles
- alignment of widgets inside widgets
- Makefile: make native on windows
- Makefile: 32bit exe
- Log warning/error when duplicate ID is detected
- Show error (+reason) when loading a pack fails
- Log facility besides stdout
- Remove toolbar and put everything inside a context menu on desktop?
- Update websocketpp, update asio (or use a different websocket implementation)
  - included websocketpp is patched to allow std::thread on mingw
  - included asio is the latest one that works with websocketpp 0.8.1
- Handle click on locations (tooltip->onClick should do something, so we don't need to move the mouse too much)
  - right click square or tooltip = full clear of accessible?
- Item: `swap_actions: <bool>` (swaps left and right mouse button, alttpr)
- Map/Locations:
  - number overlay on map (X unreachable red, Y reachable white)
- Move (more) non-shared stuff from BaseItem to JsonItem
- `Lua_Interface::PropertyMap` to get rid of ugly `Lua_Index` and `Lua_NewIndex`
- Fix UI stuff
- build with `-D_FORTIFY_SOURCE=2`, `-pie`, ASLR, RELRO ?
- fuzz lua and json interfaces

## WASM
- custom html, to only have canvas. Use js console for stdout and stderr
- sync sdl window with html canvas (size)
- mount config dir to IndexDB
  - see [here](https://stackoverflow.com/questions/54617194/how-to-save-files-from-c-to-browser-storage-with-emscripten)
    and [here](https://developer.mozilla.org/en-US/docs/Web/API/IndexedDB_API)
- implement SD2SNES/websocketpp for wasm
- implement pack loading

## UI stuff soon™
- fix the UI tree not being stable until rendered once (could be fixed with `relayout`, should be fixed in `addChild`)

## UI stuff for later
- move image loading out of widgets and just assign an abstract image that has the implementation
- some code should be moved between Widget <-> Container <-> containers
- `relayout` should probably be virtual, travel down and do stuff as expected unless overridden
- populate hgrow/vgrow in the same step as minsize/maxsize
- see [UILIB.md#Concepts](UILIB.md#Concepts)
- for correct auto-sizing we will need
  - allow variable size X/Y: `_growX`,`_growY`
  - detected min size: `_minSize`
  - manual min size
  - detected max size
  - manual max size
  - preferred size: `_autoSize` somewhat, but not in use for layout
  - actual size: `_size`
- problems regarding auto-sizing as-is
  - some widgets will change size during rendering
  - containers do not subscribe to all signals, so changing a child does not trigger relayout
  - when to `relayout` is not really decided
  - a uniform logic for relayout has to be found
  - a lot of hacks to make the current model somewhat work (see `// FIXME:`s and `// TODO:`s in source)
- Responsive UI
  - Toolbar hover/pressed
  - Pack-loading button-widget

## Random Notes
### Data additions
- Item::ID in json? Auto-generated at the moment
- Location::ID in json? Concat of names at the moment
- Unique ID for actual drops (locations' sections' checks) so we can have the same drop on two maps (detailed and overview)
- Location/section help/info (requirements to reach it)
### Other stuff
- enum class Ui::MouseButton in Mouse Events (instead of int and C-style enum)
- enum flags Ui::Map::LocationState with operator|
- (Evermizer: `settings_popup`) Layout: `header_background`
- (Evermizer) Layout: header_content: adds a button to the header
