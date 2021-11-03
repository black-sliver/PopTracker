# Packs Interface

* Pack description is in manifest.json, including available Variants.
* Everything a pack provides is done through Lua, statring at scripts/init.lua.
* Data (that is loaded through Lua) is stored as JSON inside the pack.
* Images (that are referenced in JSON) are PNGs or GIFs inside the pack.
* Most strings/identifiers are case sensitive.

### Manifest

The only things of consequence right now are:
    
    {
        "name": "<Readable Name>",
        "package_uid": "<unique string for saves>",
        "package_version": "<unique string for open and saves>",
        "platform": "<platform>",
        "variants": {
            "<some_variant_uid>": {
                "display_name": "<Readable Name>",
                "flags": [
                    "<some flag>"
                ]
            }
        }
    }

Other fields:

    {
        "game_name": "<Readable Name>",
        "author": "<Readbale Names>"
    }

Each pack can have multiple variants (e.g. map or compact).

If platform is "snes", the snes autotracker is to be enabled.

Flags are ignored at the moment.


## Lua Interface

* When calling a provided method, use `:` instead of `.`
* When loading files, the *search path* is ./ as well as ./variant/, so that

  you can have global items in `items/items.json` OR per-variant items in `[variant]/items/item.json`

The following interfaces are provided:


### global Tracker

* `string .ActiveVariantUID`: variant of the pack that is currently loaded
* `bool :AddItems(jsonfilename)`: load items from json
* `bool :AddMaps(jsonfilename)`: load maps from json
* `bool :AddLocations(jsonfilename)`: load locations from json
* `bool :AddLayouts(jsonfilename)`: load layouts from json
* `int :ProviderCountForCode(code)`: number of items that provide the code (sum of count for consumables)
* `mixed :FindObjectForCode(string)`: returns items for `code` or location section for `@location/section`
* `void :UiHint(name,string)`: sends a hint to the Ui, see [Ui Hints](#ui-hints). Only available in PopTracker, since 0.11.0


### global ScriptHost

* `bool :LoadScript(luafilename)`: load and execute a lua script
* `bool :AddMemoryWatch(name,addr,len,callback,interal)`: add a memory watch for auto-tracking, see [AUTOTRACKING.md](AUTOTRACKING.md)
* `bool :RemoveMemoryWatch(name)`: remove memory watch by name, available since 0.11.0
* `bool :AddWatchForCode(name,code,callback)`: callback(code) will be called whenever an item changed state that canProvide(code). Only available in PopTracker, since 0.11.0
* `bool :RemoveWatchForCode(name)`: remove watch by name
* `LuaItem :CreateLuaItem()`: create a LuaItem (custom item) instance


### global AutoTracker

* see [AUTOTRACKING.md](AUTOTRACKING.md)


### global ImageReference

`use ImageRef = string`
* `ImageRef :FromPackRelativePath(filename)`: for now this will just return filename and path resoltuion is done later.


### global PopVersion

a string in the form of `"1.0.0"` -- **TODO**: move to Tracker.PopVersion ?


### type LuaItem

`use ImageRef = string`
* `ImageRef .Icon`: change the icon. Use `ImageReference:FromPackRelativePath`.
* `string .IconMods`: icon modifier, see JSON's img_mods. Only available in PopTracker, since 0.11.0
* `string .Name`: item's name
* `object .ItemState`: (any) object to track internal state in lua
* `closure(LuaItem) .OnLeftClickFunc`: called when left-clicking
* `closure(LuaItem) .OnRightClickFunc`: called when right-clicking
* **TODO**: Middle, Forward, Backward or a generalized onClick(button)
* `closure(LuaItem,string code) .CanProvideCodeFunc`: called to determine if item has code
* `closure(LuaItem,string code) .ProvidesCodeFunc`: called to track progress, closure should returns 1 if code is provided (can provide && active)
* `closure(LuaItem,string code) .AdvanceToCodeFunc`: called to change item's stage to provide code (not in use yet)
* `closure(LuaItem) .SaveFunc`: called when saving, closure should return a lua object that works in LoadFunc
* `closure(LuaItem,object data) .LoadFunc`: called when loading, data as returned by `SaveFunc`
* `closure(LuaItem,key,value) .PropertyChangedFunc`: called when :Set is called and the value changed
* `:Set(key,value)`: write to property store (NOTE: property store == .ItemState)
* `:Get(key)`: read from property store. not sure what this is good for if we have `.ItemState`
* `:SetOverlay(string)`: set overlay text (see JsonItem), only available in PopTracker
* `:SetOverlayBackground(string)`: set overlay background (see JsonItem), only available in PopTracker, since 0.17.0

*Probably more to come.*


### type JsonItem

* `bool .Active`: enable/disable item
* `int .CurrentStage`: set/get stage for staged items
* `int .AcquiredCount`: set/get current amount for consumables
* `int .MaxCount`: set/get max amount for consumables (available since 0.14.0)
* `:SetOverlay(string)`: set overlay text (like count, for non-consumables), only available in PopTracker
* `:SetOverlayBackground(string)`: set background: "#000000" is RGB, "" is transparent, only available in PopTracker, since 0.17.0

*Probably more to come.*


### type LocationSection
* `.Owner`: returns empty table at the moment, see [Locations](#locations)
* `.ChestCount`: how many chests are in the section
* `.AvailableChestCount`: read/write how many chests are NOT checked


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
  + and optional on/off if `"allow_disabled": true`
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
                "disabled_img_mods": "..."
            },
            ...
        ]
    
  + img_mods filter to be applied on the img
    - `@disabled`: grey-scale
    - `overlay|path/to/img.png`: draw a second image over it
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

* `"consumable"`:
  + has count (0 = off)
  + adds `"max_quantity": 0`
  + adds optional `"initial_quantity": 0`
  + adds optional `"overlay_background": ""` set text background color with "#000000" notation
  
* `"progressive_toggle"`:
  + has on/off for each stage. Like progressive with `allow_disabled: true`.

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


### Maps

Maps are referenced by name in layouts.

    [
        {
            "name": "map_identifier",
            "location_size": 24, // size of locations on the map, unit is pixels of img
            "location_border_thickness": 2, // border around the locations
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
                            "y": 234
                        },
                        ... // having multiple locations seems to have problems?
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
                        ...
                    ]
                }
            ]
        }
    ]

**Locations:**
Each `map_location` is a square on the map and shows a popup with individual chests.

**Rules:**
Rules starting with `$` will call the lua function with that name, `@<location>/<section>` will use the result of a different access rule, other rules will just look at items' `code` (runs ProviderCountForCode(rule)).

For `$` rules, arguments can be supplied with `|`. `$test|a|b` will call `test("a","b")`.
The return value has to be a number (count).

Rules inside `[` `]` are optional (i.e. glitches work around this rule).

Rule-goups inside `{` `}` are a different set of rules to mark the section as "checkable but not collectible", marked blue on the map.

`<rule1>-<rule4>` in example above are combined as: `(<rule1> AND <rule2>) OR (<rule3> AND <rule4>)` have to be met to collect.

`{<checkrule1>, <checkrule2>}` in example above are combined as: `(<checkrule1> AND <checkrule2>)` have to be met to check.

**Sections:** 
The popup is segmented into sections.

A room with magic and 3 chests could have a section for 1 magic and a section for the 3 chests.

Depending on the map it is also possible to have 1 location per chest, but checking that off takes longer.

Name is optional and will be displayed above the section.

Sections can be addressed from Lua with `Tracker:FindObjectForCode("@location_name/secion_name")`

**Tracker Lua Interface:**

* `Tracker:FindObjectForCode('@location_name/section_name')` returns a Section or nil

**Section Lua Interface:**

* `.Owner`: probably points to location, but we return an empty table at the moment. `.Owner.ModifiedByUser` is `nil`.
* `.ChestCount`: read how many chests are in the section
* `.AvailableChestCount`: read/write how many chests are NOT checked

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
        "background":  "#xxxxxx", // background color
        "h_alignment": "{left,right,center,strech}",
        "v_alignment": "{top,bottom,center,strech}",
        "dock":        "{top,bottom,left,right}", // if inside a dock, where to dock
        "orientation": "{horizontal,vertical}", // how to orient children
        "max_height":  integer, // maximum height in px
        "max_width":   integer, // maximum width in px
        "item_margin": "<horizontal>,<vertical>", // margin in px
        "item_size":   "<horizontal>,<vertical>", // 3=default=32, 4=48, other TBD, 10+=size in pixels
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


## Ui Hints

Instead of allowing Lua direct access to the UI/Widgets, there is a "Hints" interface. See [global Tracker](#global-tracker).

* `"ActivateTab"`: value = tab name, since 0.11.0

