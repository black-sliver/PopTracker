# PopTracker Changelog

## v0.25.8

* App Features
  * Make pack selection "stick" when clicking it
  * Increase scrolling distance/speed
* Pack Features
  * SNES auto-tracking: add SA-1 mapping
  * LuaItem: add OnMiddleClickFunc
  * Add LuaLS definition file - see
    [*Lua Interface* in PACKS.md](https://github.com/black-sliver/PopTracker/blob/master/doc/PACKS.md#lua-interface)
  * Update example packs
* Fixes
  * SNES auto-tracking: fix reconnect not working properly (breaking the mapping)
  * Fix scrolling behavior in map tooltip when last element is hidden
  * Fix unnamed map tooltip section not properly updating
  * Fix handling of leading comma in json and Item.IconMods

## v0.25.7

* Fixes
  * Fix updating map locations that use `ref`(speedup in 0.25.6 broke this)

## v0.25.6

* App Features
  * Add some topics to readme: Bizhawk connector, pack install, user override, portable mode
  * Add F1 hotkey to open the readme
  * Implement user overrides for packs
  * Speedup rule evaluation when modifying locations (from Lua)
* Pack Features
  * Add support for image mods on overlay images using `|`
  * Add `^$` rule syntax to directly set AccessibilityLevel from Lua (instead of count/bool)
  * Add shorter single string rule syntax option `"access_rules": "one_code"`
  * Add smooth_map_scaling to settings.json
  * json schema for settings.json
  * Lua: properly implement require, set `...` variable to current module
  * Allow setting and reading the Tracker.BulkUpdate flag
    * automatically set while loading state
    * when set to `true`, rule evaluation is deferred until set to `false` again
* Fixes
  * Correctly encode non-ASCII paths on windows when saving (instead of crashing)
  * Lua: increase execution limit of rules to 500k instructions - some packs ran into the limit
  * Internal doc update/cleanup

## v0.25.5

* App Features
  * AP: Implement upcoming protocol changes for Archipelago (per-game IDs)
  * AP: try WSS (SSL) first, then WS (unencrypted)
  * Add F2 hotkey to open broadcast window
  * Show base item name in tooltip for badged_toggle while badge is off
* Pack Features
  * Implement initial_active_state for progressive_toggle
  * Update Lua to 5.4.6+3
  * Lua: abort (recursive) Lua `$`-rules after 100k instructions
  * Lua: add global boolean `DEBUG`, set to true to produce more verbose debug output
  * Lua: add Locations to Tracker:FindObjectForCode
  * Lua: add special "*" code for ScriptHost:AddWatchForCode to receive all code changes
* Fixes
  * Don't crash when internet is unavailable
  * Glitched + Inspect only = Unreachable
  * Fix possible crash in MemoryWatch handling
  * AP: Use embedded SSL certificates, fixing connection problems on MacOS and outdated Windows
  * Some json schema and doc fixes
  * Avoid memory leaks when throwing Lua errors from native code

## v0.25.4

* App Features
  * Add N64 auto-tracking beta for BizHawk
  * Update map tooltip (visibility) without closing/recreating it
* Pack Features
  * Add restrict_visibility_rules and force_invisibility_rules to MapLocations
  * Add initial_active_state to toggle and toggle_badged
  * Lua: implement JsonItem.Icon (for toggles only)
  * Lua: implement ImageReference.FromImageReference
  * Lua: allow returning false from MemoryWatch to have it trigger again
  * Lua: print traceback for errors in Lua rules
* Fixes
  * Warn instead of crash for invalid tabs (tabbed with no tabs)
  * Windows: avoid showing duplicates of packs
  * Don't crash for clear_as_group of multiple hosted items
  * Some json schema fixes

## v0.25.3

* App Features
  * Long hover shows tooltips in settings window
* Pack Features
  * Add `target_poptracker_version` to manifest (currently unused)
* Fixes
  * Archipelago: properly support item and location IDs > 2147483647 everywhere
  * Fix HOME/PopTracker/packs and HOME/PopTracker/assets not always being added
    to search paths on Linux and macOS
  * Properly detect if the same pack folder is listed twice in the search path
  * Linux: detect and warn if dialogs will fail due to missing dependencies
  * Linux: skip check-for-updates question if dialog might fail

## v0.25.2

* Pack Features
  * Add support for Archipelago.TeamNumber, .CheckedLocations and .MissingLocations
* Fixes
  * Clip scrollable map tooltips to expected size
  * Correctly handle clicking hosted items with clear_as_group
  * Allow boolean values for LuaItems' ProvidesCodeFunc - some packs do that
  * Don't panic when getting an unexpected value from LuaItems' ProvidesCodeFunc
  * Pack update: fix wrong % display for downloads > 42.9MB
  * Pack update: don't ask to update if latest version is already installed
  * Pack update: don't fail if backups have a naming collision
  * Pack update: don't update progress until size is known and don't crash

### Hotfix 1

* Fixes
  * Fix missing question if checking for updates should be enabled on first start

## v0.25.1

* App Features
  * Reset window position when saved value is definitely invalid
  * Use map colors from [Color Picker] (https://poptracker.github.io/color-picker.html) for map tooltips as well
* Fixes
  * Archipelago Autotracking: reapply autotracking when resetting/loading state while connected
  * Correctly apply hide_cleared_locations/hide_unreachable_locations on app start
  * Don't warn about recursion when changing item state inside Lua access rule exactly once

## v0.25.0

* App Features
  * Hide empty sections in cases where a hosted item does not exist
  * Hide map location if it has no sections
  * Make map tooltips scroll vertically on overflow
  * Automatically ping Archipelago host to keep the connection alive
* Pack Features
  * settings.json: `{ "smooth_scaling": true }` enables high quality / smooth scaling for the pack
* Fixes
  * Default itemgrid horizontal alignment to center
  * Lua: skip/ignore [BOM](https://en.wikipedia.org/wiki/Byte_Order_Mark)
    and print warning if found in a .lua file.

## v0.24.1

* Fixes
  * Fix PopTracker sometimes selecting potable mode when it shouldn't

### Hotfix 1

* Fixes
  * Fixed Archipelago SSL support - this was a build issue and only affects the Windows release build.

## v0.24.0

* App Features
  * Split-color map locations - press Ctrl+P to toggle
  * Load customizable map location colors from `%appdata%\PopTracker\colors.json` or ~/.config/PopTracker/colors.json`
  * Color Picker: https://poptracker.github.io/color-picker.html
  * Additional Hotkeys: ctrl+R to reload, ctrl+shift+R to force-reload
  * Portable mode: create `portable.txt` next to the EXE to skip using user's `%appdata%` or `~/.config`
  * Update Archipelago client lib (better support for APWorlds through data package checksum, wss/SSL support)
  * Enable Server Name Identification (SNI) on HTTPS - required to get updates from github in some regions
  * Disable tls1.1 and older for HTTPS
* Pack Features
  * Allow `min_poptracker_version` in manifest.json to disable loading of incompatible packs.

## v0.23.0

* App Features
  * Remember Archipelago auto-tracking host and slot in saved states
  * Clarify Archipelago host input, detect *some* errors
  * Different FPS limit for software renderer (default 60) and hardware renderer (default 120)
  * Include version info in Windows build (.exe -> right click -> Properties -> Details)
  * Updated `SDL2`, `SDL2_Image`, `SDL2_TTF`, `OpenSSL`, `apclientpp`
* Pack Features
  * Lua: Allow the use of .Active for progressive that can't be disabled (always true)
  * Lua: JSONItem.Type and LuaItem.Type is the type of the item as string
  * Sections can be placed into multiple locations by using `{"ref": "path/to/original", "name": "New name"}`
* Fixes
  * Fixed always ending up with software renderer on Windows (high CPU load, since 0.22.0)
  * Fix that sections with both items and hosted items could show up in the wrong color on maps
  * Update tooltip when a LuaItem changes its .Name, don't reload image if it's unchanged
  * Make some dialogs thread-safe

## v0.22.0

* App Features
  * Show stage name instead of item name when possible
  * F11 and ctrl+H to toggle location visibility
  * F5 to reload pack
  * Ctrl+F5 to force-reload pack
  * Update included libraries
* Pack Features
  * Add support for Archipelago data storage api
  * Add read access of AccessibilityLevel from Lua
  * Allow overriding specific map location size and border
  * Add support for alpha when specifying colors
  * Implement canvas
  * Implement tab icons
* Fixes
  * Correctly draw background for groups, add default

## v0.21.0

* App Features
  * Lua: replace default libs with sandboxed custom ones.
    It's still not recommended to run untrusted packs.
  * Autotracking: better naming in tooltips
  * SNES Autotracking: right-click cycles through devices (if multiple)
  * More warnings and better handling for packs doing unexpected things
  * More command line features, see `poptracker --help`
* Pack Features
  * Lua: support require (from ZIP or folder)
  * Lua: stripped-down os and io (read-only, only from current pack)
  * Support min quantity for consumables: min_quantity (JSON), MinCount (Lua)
  * Support for more things in LuaItem saves
  * Support odd ZIP variant
* Fixes
  * Fixed glitched reachable for referenced locations (`[@]` and `@->[]`)
  * Don't crash when loading invalid files as state
  * Fixed setting MaxCount from Lua
  * Fixed fuzzy pack matching not always selecting the expected one
  * Updated library versions (fixing some potential security issues)
  * Fix makeGreyscale for 24bpp

## v0.20.5

* App Features
  * New Icon by Cyb3R
  * Added support for pack updates
  * Added some CLI (command line interface) features
* Pack Features
  * Allow returning bool from lua for $-rules
  * Allow empty arrays in access/visibility rules
* Fixes
  * Fixed bug in AP UUID generation
  * Use correct bottom end of items for hit test
  * Load integrated SSL certificates from any asset dir
  * Catch exceptions when aborting HTTPs to avoid crashing

## v0.20.3

* App Features
  * Update apclient to 0.2.6

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
  * Fix vertical stretch of some widgets
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
