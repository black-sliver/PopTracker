# Packs Interface

* Pack description is in manifest.json, including available Variants.
* Everything a pack provides is done through Lua, starting at scripts/init.lua.
* Data (that is loaded through Lua) is stored as JSON inside the pack.
* Images (that are referenced in JSON) are PNGs, GIFs or JPEGs inside the pack.
* Most strings/identifiers are case sensitive.


### Manifest

`manifest.json` in the root of the pack.

The only things of consequence right now are:
    
    {
        "name": "<Readable Name>",
        "game_name": "<Readable Name>",
        "package_uid": "<unique string for saves>",
        "package_version": "<unique string for open and saves>",
        "platform": "<platform>",
        "platform_override": "<optional actual platform>",
        "variants": {
            "<some_variant_uid>": {
                "display_name": "<Readable Name>",
                "flags": [
                    "<some flag>"
                ]
            }
        },
        "versions_url": "https://example.com/versions.json",
        "min_poptracker_version": "0.23.1",
        "target_poptracker_version": "0.23.1"
    }

Other fields:

    {
        "author": "<Readbale Names>"
    }

Each pack can have multiple variants (e.g. map or compact).

`platform_override` can be used to set the platform without changing the `platform` value for compatibility reasons.
If platform is "snes", the snes autotracker is to be enabled.
If platform is "n64", the LuaConnector autotracker is to be enabled.

Currently supported flags:
* `"ap"`: pack supports Archipelago autotracking. See [AUTOTRACKING.md](AUTOTRACKING.md).
* `"apmanual"`: pack supports sending locations to Archipelago. Uses `game_name` as `game`.
* `"uat"`: pack supports UAT autotracking. See [AUTOTRACKING.md](AUTOTRACKING.md).
* `"lorom"`: (SNES) game has LoROM mapping - if not listed in gameinfo.cpp
* `"hirom"`: (SNES) game has HiROM mapping
* `"exlorom"`: (SNES) game has ExLoROM mapping
* `"exhirom"`: (SNES) game has ExHiROM mapping
* `"sa-1"`: (SNES) game has SA-1 mapping

`versions_url` can be used for automatic updates. Information from global `packs.json` takes precedence.
See https://github.com/black-sliver/PopTracker/tree/packlist for more information.

`min_poptracker_version` will check if the version is compatible when loading the pack.
It can be written as `x`, `x.y` or `x.y.z`.

`target_poptracker_version` may influence behavior to not break a pack for breaking API or behavioral changes.
There is no guarantee that everything will have a compatibility layer, but it's best practice to put the latest known
working version number there.


### Settings

Optional, `settings.json` in the root of the pack.

Configures behavior of the pack.

    {
        "smooth_scaling": true|false|null, // configure the image scaling method. null = default = currently crisp
        "smooth_map_scaling": true|false|null, // configure the image scaling method for maps. null = default = smooth
    }

NOTE: User overrides for settings are merged with the pack, replacing individual keys, not the whole file.


## Lua Interface

* When calling a provided method, use `:` instead of `.`
* When loading files, the *search path* is ./ as well as ./variant/, so that

  you can have global items in `items/items.json` OR per-variant items in `[variant]/items/item.json`

* To get proper auto-complete, type hints and warnings for the PopTracker API, you can use
  [LuaLS](https://github.com/LuaLS/lua-language-server?tab=readme-ov-file#lua-language-server)
  (VSCode/ium extension: search for `sumneko.lua`)
  with our [Definition File](../api/lua/definition/poptracker.lua).\
  You have to add a `.luarc.json` [(example)](../examples/uat-example/.luarc.json)
  with the correct path (typically `../../api/lua/definition`) to your project and restart the Language Server or IDE.\
  It is recommended to check that file into Git, but exclude it from the final pack when zipping.


The following interfaces are provided:


### global Tracker

* `string .ActiveVariantUID`: variant of the pack that is currently loaded
* `bool :AddItems(jsonfilename)`: load items from json
* `bool :AddMaps(jsonfilename)`: load maps from json
* `bool :AddLocations(jsonfilename)`: load locations from json
* `bool :AddLayouts(jsonfilename)`: load layouts from json
* `int :ProviderCountForCode(code)`: number of items that provide the code (sum of count for consumables)
* `mixed :FindObjectForCode(string)`: returns items for `code` or location section for `@location/section`
* `void :UiHint(name, value)`: sends a hint to the Ui, see [Ui Hints](#ui-hints). Only available in PopTracker, since 0.11.0
* `bool .BulkUpdate`: can be set to true from Lua to pause running logic rules.
* `bool .AllowDeferredLogicUpdate`: can be set to true from Lua to allow evaluating logic rules fewer times than items are updated.


### global ScriptHost

* `bool :LoadScript(luafilename)`: load and execute a lua script from absolute filename inside pack
  * `require` can be used instead (since PopTracker 0.21.0)
  * `require` behaves mostly like Lua require since 0.25.6
    * `"foo.baz"` will try `/scripts/foo/baz.lua`, `/scripts/foo/baz/init.lua`, `/foo/baz.lua` and `/foo/baz/init.lua`
  * `...` contains mod name for relative require since 0.25.6
* `ref :AddMemoryWatch(name,addr,len,callback,interal)`: add a memory watch for auto-tracking, see [AUTOTRACKING.md](AUTOTRACKING.md)
* `bool :RemoveMemoryWatch(name)`: remove memory watch by name, available since 0.11.0
* `ref :AddVariableWatch(name,{variables, ...},callback,interal)`: add a variable watch for auto-tracking, see [AUTOTRACKING.md](AUTOTRACKING.md)
* `bool :RemoveVariableWatch(name)`: remove variable watch by name, available since 0.16.0
* `ref :AddWatchForCode(name,code,callback)`: callback(code) will be called whenever an item changed state that canProvide(code). Only available in PopTracker, since 0.11.0, will return a reference (name) to the watch since 0.18.2. Use "*" to trigger for all codes since 0.25.5.
* `bool :RemoveWatchForCode(name)`: remove watch by name
* `LuaItem :CreateLuaItem()`: create a LuaItem (custom item) instance
* `ref :AddOnFrameHandler(name,callback)`: callback(elapsed) will be called every frame, available since 0.25.9
* `bool :RemoveOnFrameHandler(name)`: remove a frame callback
* `ref :AddOnLocationSectionChangedHandler(name, callback)`: callback (LocationSection) will be called whenever any location section changes, available since 0.26.2
* `bool :RemoveOnLocationSectionChangedHandler(name)`: removes a previously added LocationSectionChanged callback, available since 0.26.2
* `ThreadProxy :RunScriptAsync(luaFilename, arg, completeCallback, progressCallback)`: Load and run script in a separate thread. `arg` is passed as global arg. Most other things are not available in the new context. Use `return` to return a value from the script, that will be passed to `callback(result)`. (ThreadProxy has no function yet)
* `ThreadProxy :RunStringAsync(script, arg, completeCallback, progressCallback)`: same as RunScriptAsync, but script is a string instead of a filename.
* `void :AsyncProgress(arg)`: call progressCallback in main context on next frame. Arg is passed to callback.


### global AutoTracker

* see [AUTOTRACKING.md](AUTOTRACKING.md)


### global ImageReference

`use ImageRef = string`
* `ImageRef :FromPackRelativePath(filename)`: for now this will just return filename and path resolution is done later.
* `ImageRef :FromImageReference(original, mod)`: return ImageRef that is original ImageRef + mod string.


### global PopVersion

a string in the form of `"1.0.0"` -- **TODO**: move to Tracker.PopVersion ?


### global AccessibilityLevel (enum)

a table representing an enum with the following constants: \
`None`, `Partial`, `Inspect`, `SequenceBreak`, `Normal`, `Cleared`


### other globals

* `DEBUG` set to true or an array of strings to get more error or debug output
  * `false` or `nil`: disable everything
  * `true`: enable everything
  * `{'fps'}`: enable FPS output in console
  * `{'errors'}`: enable more detailed error reporting
  * `{'fps', 'errors', ...}`: enable multiple
* `require` function, see [ScriptHost:LoadScript](#global-scripthost)


### type LuaItem

`use ImageRef = string`
* `ImageRef .Icon`: change the icon. Use `ImageReference:FromPackRelativePath`.
* `string .IconMods`: icon modifier, see JSON's img_mods. Only available in PopTracker, since 0.11.0
* `string .Name`: item's name
* `object .ItemState`: (any) object to track internal state in Lua. Keys have to be strings for Get and Set below to work.
* `closure(LuaItem) .OnLeftClickFunc`: called when left-clicking
* `closure(LuaItem) .OnRightClickFunc`: called when right-clicking
* `closure(LuaItem) .OnMiddleClickFunc`: called when middle-clicking, since 0.25.8
* **TODO**: Forward, Backward or a generalized onClick(button)
* `closure(LuaItem,string code) .CanProvideCodeFunc`: called to determine if item has code
* `closure(LuaItem,string code) .ProvidesCodeFunc`: called to track progress, closure should returns 1 if code is provided (can provide && active)
* `closure(LuaItem,string code) .AdvanceToCodeFunc`: called to change item's stage to provide code (not in use yet)
* `closure(LuaItem) .SaveFunc`: called when saving, closure should return a lua object that works in LoadFunc
* `closure(LuaItem,object data) .LoadFunc`: called when loading, data as returned by `SaveFunc`
* `closure(LuaItem,key,value) .PropertyChangedFunc`: called when :Set is called and the value changed
* `:Set(string key,value)`: write to property store (NOTE: property store == .ItemState)
* `:Get(string key)`: read from property store. not sure what this is good for if we have `.ItemState`
* `:SetOverlay(string)`: set overlay text (see JsonItem), only available in PopTracker
* `:SetOverlayBackground(string)`: set overlay background (see JsonItem), only available in PopTracker, since 0.17.0
* `:SetOverlayFontSize(int)`: set overlay font size (~pixels), only available in PopTracker, since 0.17.2
* `:SetOverlayAlign(string)`: set overlay alignment to "left", "center" or "right", only available in PopTracker, since 0.25.9
* `.Owner`: returns empty table at the moment for compatibility reasons, since 0.18.2
* `.Type`: gets the type of the item as string (always "custom"), since 0.23.0
* `.IgnoreUserInput`: disables state/stage changes when clicking the item, since 0.26.2

*Probably more to come.*


### type JsonItem

* `bool .Active`: enable/disable item
* `int .CurrentStage`: set/get stage for staged items
* `int .AcquiredCount`: set/get current amount for consumables
* `int .MinCount`: set/get min amount for consumables (available since 0.21.0)
* `int .MaxCount`: set/get max amount for consumables (available since 0.14.0)
* `:SetOverlay(string)`: set overlay text (like count, for non-consumables), only available in PopTracker
* `:SetOverlayBackground(string)`: set background: "#000000" is (A)RGB, "" is transparent, only available in PopTracker, since 0.17.0
* `:SetOverlayFontSize(int)`: set overlay font size (~pixels), only available in PopTracker, since 0.17.2
* `:SetOverlayAlign(string)`: set overlay alignment to "left", "center" or "right", only available in PopTracker, since 0.25.9
* `.Owner`: returns empty table at the moment for compatibility reasons, since 0.18.2
* `.Increment`: sets or gets the increment (left-click) value of consumables
* `.Decrement`: sets or gets the decrement (right-click) value of consumables
* `.Type`: gets the type of the item as string ("toggle", ...), since 0.23.0
* `.Icon`: override displayed image, including mods, until state changes, since 0.26.2. Prefer stages instead.

*Probably more to come.*


### type LocationSection
* `.Owner`: returns empty table at the moment, see [Locations](#locations)
* `.ChestCount`: how many chests are in the section
* `.AvailableChestCount`: read/write how many chests are NOT checked
* `.AccessibilityLevel`: read-only, giving one of the AccessibilityLevel constants
* `.FullID`: read-only, full id such as "location/section"


### type Location
* `.AccessibilityLevel`: read-only, giving one of the AccessibilityLevel constants (will not give CLEARED at the moment), since 0.25.5


## JSON

### Items

    [
        {
            "name": "<Readable Name>",
            "type": "<type>",
            ...
        },
        ...
    ]
    
**Type:**
* `"progressive"`:
  + has any number of states (`"stages"`)
  + adds automatic "off" stage if `"allow_disabled": true` (default true)
  + adds optional `"loop": false` to determine if the states should loop (last → first) when clicking
  + adds optional `"initial_stage_idx": 0`
  + adds array of states (`"stages"`)
  
        "stages": [
            {
                "img": "path/to/img.png",
                "disabled_img": "path/to/disabled.png", // optional, grey img if missing
                "codes": "check_identifier",
                "secondary_codes": "state_identifier", // unused at the moment
                "inherit_codes": false,
                "img_mods": "@disabled",
                "disabled_img_mods": "...",
                "name": "optional name override" // since 0.21.1
            },
            ...
        ]
    
  + img_mods filter to be applied on the img
    - `@disabled`: grey-scale
    - `overlay|path/to/img.png|overlay_filters...`: draw a second image over it; overlay_filters are applied to overlay (since 0.25.6)
    - NOTE: order matters, applied left to right
  + inherit_codes: true will make stage3 provide codes for item, stage1, 2 and 3 (default true)

* `"toggle"`:
  + only has on/off
  + greyed-out version will be generated by the tracker
  + adds `img` and `codes` at the same level
  
          {
            ...
            "img": "path/to/img.png",
            "img_mods": "optional_filter",
            "codes": "check_identifier"
          }
  + adds `disabled_img`: override img when off
  + adds `disabled_img_mods`: like `img_mods` but for off. Defaults to `img_mods` + `"@disabled"`). Any value (e.g. `"none"`) disables defaults.
  + adds `initial_active_state`: precollected if true, since 0.25.4

+ `"static"`:
  + behaves like a toggle that is always on

* `"consumable"`:
  + has AcquiredCount (0 = grey/disabled)
  + adds `"min_quantity": 0`, since 0.21.0
  + adds `"max_quantity": 0`
  + adds `"increment": 1` can be used if an item pickup is always a certain count, since 0.18.3
  + adds `"decrement": 1` can be used if using an item always uses up multiple, since 0.18.3
  + adds optional `"initial_quantity": 0`
  + adds optional `"overlay_background": ""` set text background color with "#000000" (A)RGB notation
  + adds optional `"overlay_font_size": 0` set font size of overlay text
  + adds optional `"overlay_align": "right"` set text alignment of overlay text to "left", "right" or "center", since 0.25.9
  + adds alias `badge_font_size` for `overlay_font_size`
  
* `"progressive_toggle"`:
  + like progressive with `allow_disabled: false`, but the current stage can be toggled between "on" and "off"
  + precollected overlay if `initial_active_state` is true, since 0.25.5

* `"composite_toggle"`:
  + is linked to two items left and right that have to be defined before
  + lua access is through linked items' .Active only
  + adds `"item_left": "code"` and `"item_right": "code"`
  + adds 4 stages via
```jsonc
    "images": [
        {
            "left": false, "right": false,
            // img, mods like stage
        },
        {
            "left": true, "right": false,
            // img, mods like stage
        },
        // ...
    ]
```

* `"toggle_badged"`:
  + display `"img"` over `"base_item"`
  + toggle visibility of `"img"` with right mouse button
  + change underlying item with left mouse button
  + precollected overlay if `initial_active_state` is true, since 0.25.4
```jsonc
    {
        "name": "Some Item",
        "type": "...",
        "img": "path/to/item.png",
        "codes": "some_item"
    },
    {
        "name": "Combination's name",
        "type": "toggle_badged",
        "base_item": "some_item",
        "img": "path/to/overlay.png",
        "codes": "some_overlay"
    }
```


### Maps

Maps are referenced by name in layouts.

    [
        {
            "name": "map_identifier",
            "location_size": 24, // size of locations on the map, unit is pixels of img
            "location_border_thickness": 2, // border around the locations
            "location_shape": "rect", // or "diamond", since 0.26.2
            "img": "path/to/img.png"
        },
        ...
    ]

**TODO**: we want to add more stuff, like displaying numbers besides locations

### Locations

Locations define drops on maps, rules to have them accessible as well as the looks of it

    [
        {
            "name": "Area name",
            "short_name": "Area", // shorter version of name. currently unused
            "access_rules": [
                "<rule1>,<rule2>",
                "<rule3>,<rule4>",
                "{<checkrule1>, <checkrule2>}",
                ...
            ],
            "visibility_rules": [
                ... // like access rules but for visibility of the location or section
            ],
            "chest_unopened_img": "path/to/chest.png", // default if children do not override
            "chest_opened_img": "path/to/chest.png",
            "overlay_background": "#000000", // set background for number of unopened chests #(AA)RRGGBB
            "color": "#ff0000", // color accent/tint of the location tooltip. currently unused
            "parent": "parent's name", // read below
            "children": [
                {
                    "name": "Location or check name",
                    "access_rules": [
                        "<rule on top of parent's>",
                        ...
                    ],
                    "chest_unopened_img": "path/to/chest.png", // default if section does not override
                    "chest_opened_img": "path/to/chest.png",
                    "map_locations": [
                        {
                            "map": "map_name",
                            "x": 123,
                            "y": 234,
                            "size": 24, // override map default, since 0.21.1
                            "border_thickness": 2, // override map default, since 0.21.1
                            "shape": "rect", // override map default, since 0.26.2
                            "restrict_visibility_rules": [
                                ...  // additional visibility rules for individual map locations, since 0.26
                            ],
                            "force_invisibility_rules": [
                                ... // additional visibility rules that hide the map location if true, since 0.26
                            ]
                        }
                    ],
                    "sections": [
                        {
                            "name": "Human readable name",
                            "clear_as_group": true|false, // one click does all checks in this section
                            "chest_unopened_img": "path/to/chest.png",
                            "chest_opened_img": "path/to/chest.png",
                            "item_count": 1, // number of checks in this section (all use the same image)
                            "hosted_item": "item", // this item will be checked when cleared
                            "access_rules": [
                                "<rule on top of parent's>",
                                ...
                            ],
                            "visibility_rules": [
                                "<rule on top of parent's>",
                                ...
                            ]
                        },
                        {
                            "ref": "Path/to/another/section" // links and displays a section from another location here
                        },
                        ...
                    ]
                }
            ]
        }
    ]

**Locations:**
Each `map_location` is a square on the map and shows a popup with individual chests.

**Rules:**
Rules starting with `$` will call the Lua function with that name, `@<location>/<section>` will use the result of a different access rule, other rules will just look at items' `code` (runs ProviderCountForCode(rule)).

Rules starting with `^` interpret the value as AccessibilityLevel instead of count. That is `^$func` can directly set the AccessibilityLevel, sometimes removing the need for `[]` and `{}` (see below). Only available in PopTracker, since 0.25.6.

For `$` rules, arguments can be supplied with `|`. `$test|a|b` will call `test("a","b")`.
The return value has to be a number (count) or boolean (since v0.20.4).

Rules inside `[` `]` are optional (i.e. glitches work around this rule).

Rule-goups inside `{` `}` are a different set of rules to mark the section as "checkable but not collectible", marked blue on the map.

`<rule1>-<rule4>` in example above are combined as: `(<rule1> AND <rule2>) OR (<rule3> AND <rule4>)` have to be met to collect.

`{<checkrule1>, <checkrule2>}` in example above are combined as: `(<checkrule1> AND <checkrule2>)` have to be met to check.

Individual rules can be specified as json array instead of string, which allows to use `,` inside names or arguments. Only available in PopTracker, since 0.19.1.

Rules can be specified as a single string, which is equivalent to `[[string]]`. Only available in PopTracker, since 0.25.6.

**Parent:**
With `"parent"`, the location's parent can be overwritten. Since PopTracker v0.19.2.
This is useful to put a location from `dungeons.json` into a location from `overworld.json`,
which could also be done with `@` access rules.

**Sections:** 
The popup is segmented into sections.

A room with magic and 3 chests could have a section for 1 magic and a section for the 3 chests.

Depending on the map it is also possible to have 1 location per chest, but checking that off takes longer.

Name is optional and will be displayed above the section.

Sections can be addressed from Lua with `Tracker:FindObjectForCode("@location_name/section_name")`

`hosted_item` is a comma separated list of item codes that are always in this section.

`item_count` is the number of checks (chests, ...) and defaults to 1 unless `hosted_item` is used, in which case it defaults to 0.

`ref` make this section a reference to another section, ignoring all other properties defined here. This can be used to have the same section in detailed and overworld maps.

**Tracker Lua Interface:**

* `Tracker:FindObjectForCode('@location_name/section_name')` returns a Section or nil
* `Tracker:FindObjectForCode('@location_name')` returns a Location or nil

**Section Lua Interface:**

* `.Owner`: probably points to location, but we return an empty table at the moment. `.Owner.ModifiedByUser` is `nil`.
* `.ChestCount`: read how many chests are in the section
* `.AvailableChestCount`: read/write how many chests are NOT checked
* `.AccessibilityLevel`: read-only, giving one of the AccssibilityLevel constants

**Location Lua Interface:**

* `.AccessibilityLevel`: read-only, giving one of the AccssibilityLevel constants, since 0.25.5

**Future:**
We probably want to add a different (additional) interface to Pop for this:
* Give every chest an id
* `Tracker:setChestState(id,state)`
* we would then have pop-autotracking.lua and emo-autotracking.lua

### Layouts

* layouts define how the items and maps are placed in the UI
* layout definitions are a map of `"name" => widget` where widget can be some container, a reference, an item grid/array, a single item or a (world-)map
* the named layout `"tracker_default"` is the root object of the tracker view
* if `"tracker_default"` is missing, `"tracker_horizontal"` or `"tracker_vertical"` is used (no way for the user to pick yet)
* the named layout `"tracker_broadcast"` is the root object of the broadcast view
* the named layout `"settings_popup"` is the root object of a pack/variant specific settings window
* the named layout `"tracker_capture_item"` is not supported yet ("item picker popup" for hints)
* containers have `"content"` which can be an array of other widgets or a json object for a single other widget

the final hierarchy looks something like this: `json root -> "tracker_default" -> "content" -> "content" -> ...`

**Properties:**

    {
        "type":        "{container,dock,array,tabbed,group,item,itemgrid,map,layout,recentpins}",
        "background":  "#000000", // background color #(AA)RRGGBB
        "h_alignment": "{left,right,center,stretch}", // defaults to center for item grid
        "v_alignment": "{top,bottom,center,stretch}",
        "dock":        "{top,bottom,left,right}", // if inside a dock, where to dock
        "orientation": "{horizontal,vertical}", // how to orient children
        "max_height":  integer, // maximum height in px
        "max_width":   integer, // maximum width in px
        "margin":      "left,top,right,bottom",
        "item_margin": "<horizontal>,<vertical>", // margin in px
        "item_size":   "<horizontal>,<vertical>", // 3=default=32, 4=48, other TBD, 10+=size in pixels
        "item_h_alignment": "{left,right,center,stretch}", // align image inside item; PopTracker since 0.19.1
        "item_v_alignment": "{top,bottom,center,stretch}", // as above; stretch is not implemented for either
        "dropshadow":  bool // enable/disable drop shadow, not implemented yet
    }

**Type:**
* `"container"`: has a single child (because that's how containers work IRL)
* `"dock"`: like array, but uses `"dock"` to place children
* `"array"`: like dock, but uses `"orientation"` instead of `"dock"` to place children
* `"tabbed"`: tabs that switch between multiple children, has `"tabs": [ {"title":"...","content":{...}} ]`
* `"group"`: has a single child + a header (`"header": "text"`)
* `"item"`: a single item (`"item": "item_code"`, )
* `"itemgrid"`: has `"rows"` as an array of rows, each being an array of `item_code` strings
* `"map"`: has optional `"maps"` which is array of map ids, if missing all maps are added
* `"layout"`: pastes a different named object here (`"key": "name_of_layout"`)
* `"recentpins"`: pinned items, uses `{"style":"{wrapped,?}","compact":bool}`
* `"text"`: has property `"text": "string"` to display a string
* `"canvas"`: has width, height and content - children have width, height, canvas_top and canvas_left; since 0.21.1

**Margin:**

Margin can alternatively be specified as "x,y" or single int and is an offset of the widget's position and size.
Margin defaults to 0 for items and "inner arrays" (array inside array), 5 for everything else.


## Ui Hints

Instead of allowing Lua direct access to the UI/Widgets, there is a "Hints" interface. See [global Tracker](#global-tracker).
Hints consist of name and value. The name describes which adjustment to make, value is the value for that adjustment.

The following hint names are defined:
* `"ActivateTab"`: value = tab name, activates tab with tab name, since 0.11.0
