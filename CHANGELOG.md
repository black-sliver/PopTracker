# PopTracker Changelog

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
