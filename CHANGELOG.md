# PopTracker Changelog

## v0.20.2

* App Features
  * Allow continuous resizing of windows on Windows and Mac
* Pack Features
  * Allow non-integers in margin (ignoring the fraction)
  * Add Player number as argument to Archipelago ItemHandler
  * Add AutoTracker:GetConnectionState

## v0.20.1

* Fixes
  * Fix "AT" button sometimes being invisible
  * Re-add memory watches when toggling "AT"
  * Change background fill to cover margin
  * Fix wrong min/max size of groups
* Pack Features
  * Allow case mismatch of codes in layout json
* App Features
  * Allow multiple auto-tracking back-ends at the same time
  * Clear all checks in a location with right mouse button

## v0.20.0

* Fixes
  * Detect access/visibility rule recursion
  * Fix order of events for Archipelago auto-tracking
  * Make consumable decrement step default to increment step
* App Features
  * Automatically switch between horizontal and vertical layout
  * Limit maximum min window size based on screen size
  * Limit window size to 96x96 - 8kx4k
  * Add drag&drop support for packs and saves into the main window
  * Add option to set usb2snes ip/host to PopTracker.json
* Pack Features
  * Add support for more legacy packs
  * Add support for `margin`
  * Add partial support for `h_alignment` in item grids
  * Add `item_h_alignment`
  * Allow rules to be arrays instead of comma separated strings
  * Allow most integers to be in quotes in json

## v0.19.0

* Fixes
  * Fix modifying watches from watch callbacks
  * Fix vertical strech of some widgets
  * Fix/ignore composite toggles with bad left/right
  * Fix what codes composite toggles provide
  * Fix setting progressive with allow_disabled from Lua
  * Change progressive with allow_disabled to what people expect
* App Features
  * Actually reload pack with the reload button when files changed
  * Make pack list scrollable
  * Make snes auto-tracking work for SRAM (requires usb2snes mapping)
* Pack Features
  * Add support for consumable increment and decrement
  * Added snes memory mapping flags (hirom, lorom, exhirom, exlorom) for AT
  * Support @-rules for locations (not just sections)
  * More lenient typing from Lua and json
  * Change return value of `Add*Watch*` to be a watch reference (currently name)
  * Allow saving/loading nil for Lua items
  * Make overlay's/badge's pink background transparent
  * Partial support for 'legacy' packs

## v0.18.2

* Fixes
  * Fix some locations not updating when loading/resetting state
  * Fix "glitch-reachable" for @-access_rules
  * Fixed multiple issues handling Archipelago connect errors and disconnect
* App Features
  * Win32: enable "visual style" (fancier MessageBoxes)
  * Remember last import/export file when loading state
* Pack Features
  * Return empty table for Item.Owner to support more packs

## v0.18.1

* Fixes
  * Warn instead of crash when trying to call a missing Lua function
  * Fix crash when cancelling import/load state
  * Fix some minor memory leaks
* App Features
  * Added preliminary support for Archipelago Multiworld connection
  * Allow most texts to break into multiple lines ('\n')
  * Allow tabs to break into multiple lines (automatically)
  * Auto-save every 60 seconds
* Pack Features
  * Add support for text nodes
  * Add support for overlay_background in location items

## v0.18.0

* Fixes
  * Fix a bug in rules parsing (accessibility used as visibility)
  * Fix a memory leak when unloading pack
  * Fix indexed images with overlay not rendering transparent
* App Features
  * Option to exclude pre-installed packs
  * Option to check for updates
  * Export/Import current state
  * Support for custom FPS limit:
    run+exit app, modify `%APPDATA%\PopTracker\PopTracker.json`: `"fps_limit":<value>`
  * Better debug output for some errors / warnings
* Pack Features
  * Add support for `toggle_badged` item type

## v0.17.2

* Fixes
  * save and restore overlay background for JSON items
* Pack Features
  * allow setting overlay font size through JSON (`overlay_font_size`)
    and Lua (`SetOverlayFontSize()`)
  * alias `badge_font_size` for `overlay_font_size`

## v0.17.1

* Fixes
  * Fix y position of items in item grid
  * Fix UAT-related crash when changing packs/variants
* App Features
  * Hide "AT" if no auto-tracking backend is available

## v0.17.0

* Fixes
  * Make clear_as_group also check hosted_item
  * Make clear_as_group default to true
* Pack Features
  * Allow Lua "codes" (`$...`) in ProviderCountForCode
  * Add support for visibility_rules
  * Add overlay_background for consumables and LuaItems

## v0.16.0

* Fixes
  * Don't access SNES ROM if not supported
  * Fix location border not being drawn
  * Sane defaults for location size and border
* App Features
  * Add support for UAT protocol
  * Click on "AT" to toggle auto-tracking
  * Add dummy variables to Items for Lua to get more packs to load

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
