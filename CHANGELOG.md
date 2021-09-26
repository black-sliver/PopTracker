# PopTracker Changelog

## v0.16.0

* Fixes
  * Don't access SNES ROM if not supported
  * Fix location border not being drawn
  * Sane defaults for location size and border
* App Features
  * Add support for UAT protocol
  * Click on "AT" to toggle auto-tracking
  * Add dummy variables to Items for LUA to get more packs to load

## v0.15.0

* Fixes
  * Fix crash when loading packs on older CPUs:
    make liblua build not target a specific CPU
* App Features
  * More verbose debug output
  * Use --console to open a dos prompt for debug output on windows
  * Debug output to terminal/dos prompt when starting from terminal
  * Debug output to file:
    run+exit app, modify `%APPDATA%\PopTracker\PopTracker.json`: `"log":true` to `"log":false`, run app
  * Support for software renderer:
    run+exit app, modify `%APPDATA%\PopTracker\PopTracker.json`: `"software_renderer":true` to `"software_renderer":false`, run app
  * Don't disable compositing in X11

## v0.14.3

* Fixes
  * make static items default to "on" for access_rules

## v0.14.2

* Pack Features
  * work around bug in IoGR's logic with random starting location
  * added `composite_toggle` item type

## v0.14.1

* Fixes
  * Make items' pink background transparent (disabled/with img_mods)

## v0.14.0

* App Features
  * Accept mouse clicks when window is not focused
  * Add support for zipped packs (no need to unpack anymore)
  * Auto-save should now always work
  * More debug output
* Pack Features
  * arguments to lua access rules
  * `JsonItem.MaxCount` now readable and writable
  * dummy `LocationSection.CapturedItem` to allow packs to load
  * Make items' pink background transparent
* Fixes
  * don't let SNES auto-tracking block unloading packs
  * fix bug in creating (auto-save) directories
  * fix a memory leak when creating directories

## v0.12.1

* Fixes
  * fix potential crash when changing packs/variants

## v0.12.0

* App Features
  * macOS support
  * allow locations to be on multiple points on a map
* Pack Features
  * support more layouts/packs
* Fixes
  * fix crash when listing a pack with no variants
  * work around slow shutdown for watch with slow update interval

## v0.11.0

* App Features
  * `--version` command line switch
  * save and restore ui state/hints (active tabs)
* Pack Features
  * `LuaItem.IconMods`
  * `ScriptHost:RemoveMemoryWatch`
  * `ScriptHost:AddWatchForCode`
  * `ScriptHost:RemoveWatchForCode`
  * `Tracker:UiHint`
* Fixes
  * fix some potential crashes from lua
  * fix a crash when closing
  * fix restoring window position on win32 
  * sanitize save paths
  * changed sizing in dock, hbox and vbox (hopefully for the better)
  * fix memory leaks for lua


## v0.1.0

* initial release
