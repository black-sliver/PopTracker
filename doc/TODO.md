# PopTracker TODO

Check [OUTLINE.md](OUTLINE.md) to get an overview of everything.

## Help wanted
- OSX .dmg
- Fancier GUI
  - Better widgets ("real" buttons)
  - Nicer icons (I'm not an artist)

## General Stuff
- isReachable optimizations:
    - invalidate only parts of _reachableCache when toggling items?
    - update location state on next frame instead of immediately (start of auto-tracking)
    - try to determine if location state needs update?
- show version somewhere in the app
- MemoryWatch: only run callback if all bytes of a watch have been read to avoid potential races (this has to add race-free bool to SNES::readBlock)
- pins
- notes
- alignment of widgets inside widgets
- Show error (+reason) when loading a pack fails
- Log/console/error window as part of the UI (currently --console and stdout in shell/prompt exists)
- Remove toolbar and put everything inside a context menu on desktop?
- Update websocketpp, update asio
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
  - see [ap-soeclient](https://github.com/black-sliver/ap-soeclient/) as an example
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
  - Pack-loading button-widget

## Random Notes
### Data additions
- Item::ID in json? Auto-generated at the moment
- Location::ID in json? Concat of names at the moment
- Unique ID for actual drops (locations' sections' checks) so we can have the same drop on two maps (detailed and overview)
- Location/section help/info/hover text (requirements to reach it)
### Other stuff
- enum flags Ui::Map::LocationState with operator|
- (Evermizer: `settings_popup`) Layout: `header_background`
- (Evermizer) Layout: header_content: adds a button to the header
