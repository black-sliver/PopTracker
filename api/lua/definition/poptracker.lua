---@meta

--
-- PopTracker API definition file.
-- Do NOT load this into your pack.
-- Read doc/PACKS.md#lua-interface for instructions.
--

---- Helpers ----

---@alias AnyObject JsonItem | LuaItem | LocationSection | Location

---Options for extra debug output
--- - fps: enable FPS output in console
--- - errors: enable more detailed error reporting
---@see DEBUG
---@alias DebugOption "fps"|"errors"

---- Globals ----

---Currently running PopTracker version as string "x.y.z".
---@type string
PopVersion = "0.33.1"
-- Actual value comes from the program, not from here, but try to keep in sync with API version here.

---Set to true or an array of strings to get more error or debug output.
---@see DebugOption
---@type nil | boolean | (DebugOption)[]
DEBUG = false

---If defined in global scope, called when a game is connected (currently memory interfaces only).
---@type fun():nil
autotracker_started = nil

---If defined in global scope, called when auto-tracking is disabled by the user (currently memory interfaces only).
---@type fun():nil
autotracker_stopped = nil


---- Tracker ----

---@class Tracker
Tracker = {}

---@type string
Tracker.ActiveVariantUID = ""

---load items from json
---@param jsonFilename string file to load, relative to variant folder or root of the pack (will try both)
---@return boolean true on success
function Tracker:AddItems(jsonFilename) end

---load maps from json
---@param jsonFilename string file to load, relative to variant folder or root of the pack (will try both)
---@return boolean true on success
function Tracker:AddMaps(jsonFilename) end

---load locations from json
---@param jsonFilename string file to load, relative to variant folder or root of the pack (will try both)
---@return boolean true on success
function Tracker:AddLocations(jsonFilename) end

---load layouts from json
---@param jsonFilename string file to load, relative to variant folder or root of the pack (will try both)
---@return boolean true on success
function Tracker:AddLayouts(jsonFilename) end

---get provided count for an (item) code
---@param code string the (item) code to look up
---@return integer count of items providing the code or the sum of count for consumables
function Tracker:ProviderCountForCode(code) end

---get item for `code`, location for `@location` or location section for `@location/section`
---@param code string the code to look up
---@return AnyObject? object that matches the code
function Tracker:FindObjectForCode(code) end

---@alias UiHintName
---| "ActivateTab" # Activates tab with tab name = hint value. Available since 0.11.0.

---Sends a hint to the UI, see https://github.com/black-sliver/PopTracker/blob/master/doc/PACKS.md#ui-hints .
---Only available in PopTracker, since 0.11.0.
---@param name UiHintName which adjustment the UI should do
---@param value string value or argument for the hint
---@return nil
function Tracker:UiHint(name, value) end

---pause evaluating logic rules when set to true
---NOTE: Since failing to set BulkUpdate back to false will stop PopTracker from updating, make sure your code between setting BulkUpdate is error resistant (for example by using pcall).
---@type boolean
Tracker.BulkUpdate = false

---Allow to evaluate logic rules fewer times than items are updated.
---Only available in PopTracker, since 0.28.1.
---Since 0.31.0 defaults to true for flag 'ap' and gets initialized from settings.json. Honors target_poptracker_version.
---Use as: `if Tracker.AllowDeferredLogicUpdate ~= nil then Tracker.AllowDeferredLogicUpdate = true end`
---@type boolean
Tracker.AllowDeferredLogicUpdate = false


---- ScriptHost ----

---@class ScriptHost
ScriptHost = {}

---Load and execute a Lua script from absolute filename inside pack.
---* sets `...` variable to mod name (for relative require) since 0.25.6
---* require can be used instead since 0.25.6 (0.21.0 in some cases)
---    * `require "foo.baz"` will try `/scripts/foo/baz.lua`, `/scripts/foo/baz/init.lua`,
---    `/foo/baz.lua` and `/foo/baz/init.lua`
---    * LoadScript needs an "absolute" path (including `scripts/`) instead
---    * LoadScript needs to use globals, require can return for `local x = require "foo"`
---* require is recommended if backwards compatibility is not required
---@param luaFilename string file to run, relative to variant folder or root of the pack (will try both)
---@return boolean true on success
function ScriptHost:LoadScript(luaFilename) end

---@alias ThreadProxy table  -- to be defined

---Execute a Lua script in a separate thread.
---Arg passed here has to be json serializable and will be made available as global arg to the script.
---Once the script finishes, callback will be called on the next frame in the host context with the return value of the script as argument.
---@param luaFilename string file to run, relative to variant folder or root of the pack (will try both)
---@param arg any passed as global arg to the new script
---@param completePallback fun(result:any):nil called when the script finishes
---@param progressCallback fun(progress:any)? optional, called when the script calls ScriptHost:AsyncProgress
---@return ThreadProxy TBD
function ScriptHost:RunScriptAsync(luaFilename, arg, completePallback, progressCallback) end

---Same as RunScriptAsync, but run string instead of file.
---@see ScriptHost.RunScriptAsync
---@param arg any passed as global arg to the new script
---@param completePallback fun(result:any):nil called when the script finishes
---@param progressCallback fun(progress:any)? optional, called when the script calls ScriptHost:AsyncProgress
---@return ThreadProxy TBD
function ScriptHost:RunStringAsync(script, arg, completePallback, progressCallback) end


---Queue call to main Lua's progressCallback with arg on next frame. Can only be used in async context/thread.
---@param arg any passed to progressCallback, has to be json serializable
function ScriptHost:AsyncProgress(arg) end


---add a memory watch for auto-tracking, see [AUTOTRACKING.md](https://github.com/black-sliver/PopTracker/blob/master/doc/AUTOTRACKING.md)
---@param name string identifier/name of this watch
---@param addr integer start address of memory block to watch
---@param len integer length of memory block to watch in bytes
---@param callback fun(segment:Segment):nil called when watched memory changes
---@param interval integer? milliseconds
---@return string reference for RemoveMemoryWatch
function ScriptHost:AddMemoryWatch(name, addr, len, callback, interval) end

---remove memory watch by name/reference, available since 0.11.0
---@param name string
---@return boolean true on success
function ScriptHost:RemoveMemoryWatch(name) end

---callback(code) will be called whenever an item changed state that canProvide(code).
---Only available in PopTracker, since 0.11.0,
---@param name string identifier/name of this watch
---@param code string Code to watch for. Use `"*"` for *all* codes since 0.25.5.
---@param callback fun(code:string):nil called when watched (item) code changes
---@return string reference for RemoveWatchForCode since 0.18.2
function ScriptHost:AddWatchForCode(name, code, callback) end

---remove watch for code by name/reference, available since 0.18.2
---@param name string
---@return boolean true on success
function ScriptHost:RemoveWatchForCode(name) end

---callback(store, {variableName, ...}) will be called whenever a remote variable changes.
---See [AUTOTRACKING.md](https://github.com/black-sliver/PopTracker/blob/master/doc/AUTOTRACKING.md#variable-interface-uat)
-- and [UAT README.md](https://github.com/black-sliver/UAT/blob/master/README.md) for more info.
---@param name string identifier/name of this watch
---@param variables string[] array of variable names
---@param callback fun(store:VariableStore, changedKeysArray:table):nil called when any watched variable changes
---@param interval integer? optional, currently unused
---@return string reference for RemoveVariableWatch since 0.18.2
function ScriptHost:AddVariableWatch(name, variables, callback, interval) end

---remove variable watch by name/reference
---@param name string
---@returns boolean true on success
function ScriptHost:RemoveVariableWatch(name) end

---create a new LuaItem
---@return LuaItem 
function ScriptHost:CreateLuaItem() end

---Add a handler/callback that runs on every frame.
---Available since 0.25.9.
---@param name string identifier/name of this callback
---@param callback fun(elapsedSeconds:number):nil called every frame, argument is elapsed time in seconds since last call
---@return string reference for RemoveOnFrameHandler
function ScriptHost:AddOnFrameHandler(name, callback) end

---Remove a handler/callback added by AddOnFrameHandler(name).
---Available since 0.25.9.
---@param name string identifier/name of the handler to remove
---@return boolean true on success
function ScriptHost:RemoveOnFrameHandler(name) end

---Add a handler/callback that runs when a location section changed (checks checked/unchecked).
---Any kind of filtering and detection what changed has to be done inside the callback.
---Available since 0.26.2.
---@param name string identifier/name of this callback
---@param callback fun(section:LocationSection):nil called when any location section changed
---@return string reference for RemoveOnLocationSectionChangedHandler
function ScriptHost:AddOnLocationSectionChangedHandler(name, callback) end

---Old name of RemoveOnLocationSectionChangedHandler.
---Available since 0.26.2.
---Use RemoveOnLocationSectionChangedHandler instead if you support only PopTracker >= 0.33.1.
---@see ScriptHost.RemoveOnLocationSectionChangedHandler
---@param name string identifier/name of the handler to remove
---@return boolean true on success
function ScriptHost:RemoveOnLocationSectionHandler(name) end

---Remove a handler/callback added by AddOnLocationSectionChangedHandler.
---Available since 0.33.1.
---Use RemoveOnLocationSectionHandler instead if you support PopTracker < 0.33.1.
---@see ScriptHost.RemoveOnLocationSectionHandler
---@param name string identifier/name of the handler to remove
---@return boolean true on success
function ScriptHost:RemoveOnLocationSectionChangedHandler(name) end

---- AutoTracker ----

---@class AutoTracker
AutoTracker = {}

---read 8bit unsigned integer, may return 0 if not yet cached
---@param baseAddr integer address to read
---@param offset integer? added to baseAddr to get the final address
---@return integer value at address
function AutoTracker:ReadU8(baseAddr, offset) end

---read 16bit unsigned integer, may return 0 if not yet cached
---@param baseAddr integer address to read
---@param offset integer? added to baseAddr to get the final address
---@return integer value at address
function AutoTracker:ReadU16(baseAddr, offset) end

---read 24bit unsigned integer, may return 0 if not yet cached
---@param baseAddr integer address to read
---@param offset integer? added to baseAddr to get the final address
---@return integer value at address
function AutoTracker:ReadU24(baseAddr, offset) end

---read 32bit unsigned integer, may return 0 if not yet cached
---@param baseAddr integer address to read
---@param offset integer? added to baseAddr to get the final address
---@return integer value at address
function AutoTracker:ReadU32(baseAddr, offset) end

---currently an empty table for compatibility
---@type table
AutoTracker.SelectedConnectorType = {}

---Returns what the remote (UAT) sent for the variableName.<br/>
---**This is deprecated** and it's recommended to use `store:ReadVariable` in a callback instead.
---Use `if ScriptHost.AddVariableWatch then` to detect if UAT is available in this version of PopTracker.
---@deprecated
---@param variableName string
---@return any value of variable or nil
function AutoTracker:ReadVariable(variableName) end

---@alias AutoTrackingBackendName "SNES" | "UAT" | "AP"

---@alias AutoTrackingState
---| -1 # Unavailable
---| 0 # Disabled
---| 1 # Disconnected
---| 2 # Socket Connected (not ready)
---| 3 # Game/Console connected (ready)

---get an integer corresponding to the current state of an auto-tracking backend.
-- Available since 0.20.2.
---@param backendName AutoTrackingBackendName name of the backend (case sensitive)
---@return AutoTrackingState state of the backend
function AutoTracker:GetConnectionState(backendName) end


---- Segment ---

---NOTE: the segment API is provided by `AutoTracker`, but this is not backwards compatible.
-- `ReadU8` should only be used on global AutoTracker and `ReadUInt8` on segment from watch callback.
---@class Segment
Segment = {}

---read 8bit unsigned integer, may return 0 if used on AutoTracker instead of Segment
---@param address integer (absolute) address to read
---@return integer value at address
function Segment:ReadUInt8(address) end

---read 16bit unsigned integer, may return 0 if used on AutoTracker instead of Segment
---@param address integer (absolute) address to read
---@return integer value at address
function Segment:ReadUInt16(address) end

---read 24bit unsigned integer, may return 0 if used on AutoTracker instead of Segment
---@param address integer (absolute) address to read
---@return integer value at address
function Segment:ReadUInt24(address) end

---read 32bit unsigned integer, may return 0 if used on AutoTracker instead of Segment
---@param address integer (absolute) address to read
---@return integer value at address
function Segment:ReadUInt32(address) end


---- VariableStore ----

---NOTE: the store API is provided by `AutoTracker`, but this may change in the future.
---@class VariableStore
VariableStore = {}

---Returns what the remote (UAT) sent for the variableName.
---@param variableName string
---@return any value of variable or nil
function VariableStore:ReadVariable(variableName) end


---- Archipelago ----

---@class Archipelago
Archipelago = {}

---@enum archipelagoCientStatus
Archipelago.ClientStatus = {
    UNKNOWN = 0,
    READY = 10,
    PLAYING = 20,
    GOAL = 30,
}

---The slot number of the connected player or -1 if not connected.
---@type integer
Archipelago.PlayerNumber = -1

---The team number of the connected player, -1 if not connected or `nil` if unsupported.
---Supported since 0.25.2.
---@type integer
Archipelago.TeamNumber = -1

---Array of already checked location IDs or `nil` if unsupported.
---Supported since 0.25.2.
---@type integer[]
Archipelago.CheckedLocations = {}

---Array of unchecked/missing location IDs or `nil` if unsupported.
---Supported since 0.25.2.
---@type integer[]
Archipelago.MissingLocations = {}

---The seed name of the connected room or nil if not connected.
---Supported since v0.33.0
---@type string?
Archipelago.Seed = nil

---Add callback to be called when connecting to a (new) server and state should be cleared.
---@param name string identifier/name of this handler (for debugging)
---@param callback fun(slotData:{ [string]: any }):nil
---@return boolean true on success
function Archipelago:AddClearHandler(name, callback) end

---Add callback to be called when called when an item is received.
---(player_number not in callback before 0.20.2)
---@param name string identifier/name of this handler (for debugging)
---@param callback fun(index:integer, itemID:integer, itemName:string, playerNumber:integer):nil
---@return boolean true on success
function Archipelago:AddItemHandler(name, callback) end

---Add callback to be called when a location was checked.
---@param name string identifier/name of this handler (for debugging)
---@param callback fun(locationID:integer, locationName:string):nil
---@return boolean true on success
function Archipelago:AddLocationHandler(name, callback) end

---Add callback to be called when a location was scouted.
---@param name string identifier/name of this handler (for debugging)
---@param callback fun(locationID:integer, locationName:string, itemID:integer, itemName:string, itemPlayer:integer):nil
---@return boolean true on success
function Archipelago:AddScoutHandler(name, callback) end

---Add callback to be called when the server sends a bounce.
---@param name string identifier/name of this handler (for debugging)
---@param callback fun(message:{ [string]: any }):nil called when receiving a bounce
---@return boolean true on success
function Archipelago:AddBouncedHandler(name, callback) end

---Add callback to be called when the server replies to Get.
---@param name string identifier/name of this handler (for debugging)
---@param callback fun(key:string, value:any):nil
---@return boolean true on success
function Archipelago:AddRetrievedHandler(name, callback) end

---Add callback to be called when a watched data storage value is changed.
---@param name string identifier/name of this handler (for debugging)
---@param callback fun(key:string, value:any, oldValue:any):nil
---@return boolean true on success
function Archipelago:AddSetReplyHandler(name, callback) end

---Ask the server for values from data storage, run this from a ClearHandler, keys is an array of strings.
---@param keys string[] keys to get
---@return boolean true on success
function Archipelago:Get(keys) end

---Ask the server to notify when a data storage value is changed.
---Run this from a ClearHandler, keys is an array of strings.
---@param keys string[] keys to watch
---@return boolean true on success
function Archipelago:SetNotify(keys) end

---Send locations as checked to the server.
---Supported since 0.26.2, only allowed if "apmanual" flag is set in manifest.
---@param locations integer[] locations to check
---@return boolean true on success
function Archipelago:LocationChecks(locations) end

---Send locations as scouted to the server.
---Supported since 0.26.2, only allowed if "apmanual" or "aphintgame" flag is set in manifest.
---@param locations integer[]
---@param sendAsHint integer
---@return boolean true on success
function Archipelago:LocationScouts(locations, sendAsHint) end

---Send client status to the server. This is used to send the goal / win condition.
---Supported since 0.27.1, only allowed if "apmanual" flag is set in manifest.
---@param status archipelagoCientStatus
---@return boolean true on success
function Archipelago:StatusUpdate(status) end

---Get alias/name of a player.
---Supported since 0.28.1.
---@param slot integer
---@return string alias of the player or "Unknown" if not found
function Archipelago:GetPlayerAlias(slot) end

---Get game name of a player.
---Supported since 0.28.1.
---@param slot integer
---@return string name of the game or "Unknown" if not found
function Archipelago:GetPlayerGame(slot) end

---Get item name for a specific game.
---Supported since 0.28.1.
---@param id integer item id
---@param game string name of the game
---@return string name of the item or "Unknown" if not found
function Archipelago:GetItemName(id, game) end

---Get item name for a specific game.
---Supported since 0.28.1.
---@param id integer item id
---@param game string name of the game
---@return string name of the location or "Unknown" if not found
function Archipelago:GetLocationName(id, game) end


---- ImageRef ----

---reference to a single image (currently a string)
---@class ImageRef
ImageRef = {}


---- ImageReference ----

---utility to create `ImageRef`s
---@class ImageReference
ImageReference = {}

---Create an image reference from filename.
---Note: currently path resolution happens when the ImageRef is being used, so there is no way to detect errors.
---@param filename string
---@return ImageRef reference to the image for filename
function ImageReference:FromPackRelativePath(filename) end

---Create an image reference with mods applied on top of another image reference.
---@param original ImageRef the base image
---@param mod string Mod(s) to apply. Separate with ',' for multiple.
---@return ImageRef reference to the new image
function ImageReference:FromImageReference(original, mod) end


---- AccessibilityLevel (enum) ----

---@enum accessibilityLevel
AccessibilityLevel = {
    None = 0,
    Partial = 1,
    Inspect = 3,
    SequenceBreak = 5,
    Normal = 6,
    Cleared = 7,
}

---- Highlight (enum) ----

---@enum highlight
Highlight = {
    Avoid = -1,
    None = 0,
    NoPriority = 1,
    Unspecified = 2,
    Priority = 3,
}


---- LuaItem ----

---@class LuaItem
LuaItem = {}

---Name of the item, displayed in tooltip.
---@type string
LuaItem.Name = ""

---Get or set the item's image. Use `ImageReference:FromPackRelativePath` to create an `ImageRef`.
---@type ImageRef?
LuaItem.Icon = nil

---Icon modifier, see JSON's img_mods. Only available in PopTracker, since 0.11.0.
---@type string
LuaItem.IconMods = ""

---Optional container to store item's state. Keys have to be string for `:Get` and `:Set` to work.
---@type table?
LuaItem.ItemState = nil

---At the moment (since 0.18.2) an empty table for compatibility reasons.
---@type table
LuaItem.Owner = {}

---Type of the item (since 0.23.0). Always "custom" for LuaItem.
---@type string
LuaItem.Type = "custom"

---Called to match item to code (to place it in layouts).
---Note: LuaItems have to be created before the corresponding layout is loaded.
---@type fun(self:LuaItem, code:string):boolean
LuaItem.CanProvideCodeFunc = nil

---Called to check if item provides code for access rules (ProviderCountForCode).
---@type fun(self:LuaItem, code:string):boolean
LuaItem.ProvidesCodeFunc = nil

---Called to change item's stage to provide code (not in use yet).
---@type fun(self:LuaItem, code:string)
LuaItem.AdvanceToCodeFunc = nil

---Called when item is left-clicked.
---@type fun(self:LuaItem):nil
LuaItem.OnLeftClickFunc = nil

---Called when item is right-clicked.
---@type fun(self:LuaItem):nil
LuaItem.OnRightClickFunc = nil

---Called when item is middle-clicked.
---PopTracker, since 0.25.8.
---@type fun(self:LuaItem):nil
LuaItem.OnMiddleClickFunc = nil

---Called when saving. Should return a Lua object that works in `LoadFunc`.
---@type fun(self:LuaItem):any
LuaItem.SaveFunc = nil

---Called when loading, data as returned by `SaveFunc`.
---@type fun(self:LuaItem, data:any):nil
LuaItem.LoadFunc = nil

---Called when :Set was called and the value changed.
---Can be used to update `.Icon`, etc. from `.ItemState` or `:Get()`.
---@type fun(self:LuaItem, key:string, value:any):nil
LuaItem.PropertyChangedFunc = nil

---Write to property store. If the value changed, this will call `.PropertyChangedFunc`.
---NOTE: if `.ItemState` is set, that is the property store, otherwise there is an automatic one.
---@param key string
---@param value any
function LuaItem:Set(key, value) end

---Read from property store.
---NOTE: if `.ItemState` is set, that is the property store and you can use `.ItemState` directly instead.
---@param key string
---@return any value
function LuaItem:Get(key) end

---Set item overlay text (like count, but also for non-consumables).
---Only available in PopTracker.
---@param text string
function LuaItem:SetOverlay(text) end

---Set item overlay text color (default context dependent).
---PopTracker, since 0.31.0.
---@param color string "#rgb", "#rrggbb", "#argb", "#aarrggbb" or "" (default)
function LuaItem:SetOverlayColor(color) end

---Set item overlay background color (default transparent).
---PopTracker, since 0.17.0.
---@param background string "#rgb", "#rrggbb", "#argb", "#aarrggbb" or "" (transparent)
function LuaItem:SetOverlayBackground(background) end

---Set font size for item overlay text in ~pixels.
---PopTracker, since 0.17.2
---@param fontSize integer
function LuaItem:SetOverlayFontSize(fontSize) end

---@alias HAlign "left" | "center" | "right"

---Set item overlay text alignment (default right).
---PopTracker, since 0.25.9
---@param align HAlign "horizontal alignment of overlay text"
function LuaItem:SetOverlayAlign(align) end

---Set item overlay text, same as SetOverlay.
---Since PopTracker 0.31.0.
---@see LuaItem.SetOverlay
---@type string
LuaItem.BadgeText = ""

---Set item overlay text color, same as SetOverlayColor.
---Since PopTracker 0.31.0.
---@see LuaItem.SetOverlayColor
---@type string
LuaItem.BadgeTextColor = ""


---- JsonItem ----

---@class JsonItem
JsonItem = {}

---Get or set the item's image. Use `ImageReference:FromPackRelativePath` to create an `ImageRef`.
---For performance reasons, using a staged item is recommended.
-- Available since 0.26.0. Changed to include mods and reset on change in 0.26.2.
---@type ImageRef?
JsonItem.Icon = nil

---Enable/disable item
---@type boolean
JsonItem.Active = false

---Set/get stage for staged items.
---@type integer
JsonItem.CurrentStage = 0

---Set/get current amount for consumables.
---@type integer
JsonItem.AcquiredCount = 0

---Set/get min amount for consumables.
---Available since 0.21.0.
---@type integer
JsonItem.MinCount = 0

---Set/get max amount for consumables.
---Available since 0.14.0.
---@type integer
JsonItem.MaxCount = 0

---Set/get the increment (left-click) value of consumables.
---@type integer
JsonItem.Increment = 1

---Set/get the decrement (right-click) value of consumables.
JsonItem.Decrement = 1

---At the moment (since 0.18.2) an empty table for compatibility reasons.
---@type table
JsonItem.Owner = {}

---Gets the type of the item as string ("toggle, ...").
---Available since 0.23.0.
JsonItem.Type = ""

---Disables state/stage changes when clicking the item.
---Available since 0.26.2.
---@type boolean
JsonItem.IgnoreUserInput = false

---Set item overlay text (like count, but also for non-consumables).
---Only available in PopTracker.
---@param text string
function JsonItem:SetOverlay(text) end

---Set item overlay text color (default context dependent).
---PopTracker, since 0.31.0.
---@param color string "#rgb", "#rrggbb", "#argb", "#aarrggbb" or "" (default)
function JsonItem:SetOverlayColor(color) end

---Set item overlay background color (default transparent).
---PopTracker, since 0.17.0.
---@param background string "#rgb", "#rrggbb", "#argb", "#aarrggbb" or "" (transparent)
function JsonItem:SetOverlayBackground(background) end

---Set font size for item overlay text in ~pixels.
---PopTracker, since 0.17.2
---@param fontSize integer
function JsonItem:SetOverlayFontSize(fontSize) end

---Set item overlay text / consumable amount alignment (default right).
---PopTracker, since 0.25.9
---@param align HAlign "horizontal alignment of overlay text"
function JsonItem:SetOverlayAlign(align) end

---Set item overlay text, same as SetOverlay.
---Since PopTracker 0.31.0.
---@see JsonItem.SetOverlay
---@type string
JsonItem.BadgeText = ""

---Set item overlay text color, same as SetOverlayColor.
---Since PopTracker 0.31.0.
---@see JsonItem.SetOverlayColor
---@type string
JsonItem.BadgeTextColor = ""

---- Location ----

---@class Location
Location = {}

---Read-only, giving one of the `AccssibilityLevel` constants (will not give CLEARED at the moment).
---Available since 0.25.5.
---@type accessibilityLevel
Location.AccessibilityLevel = 0


---- LocationSection ----

---@class LocationSection
LocationSection = {}

---Is an empty table at the moment for compatibility reasons.
---@type table
LocationSection.Owner = {}

---Read-only, total number of Chests in the section.
---@type integer
LocationSection.ChestCount = 1

---Read/write how many chests are NOT checked/opened.
---@type integer
LocationSection.AvailableChestCount = 1

---Read-only, giving one of the `AccessibilityLevel` constants.
---@type accessibilityLevel
LocationSection.AccessibilityLevel = 0

---Read-only, returning the full id as string such as "Location/Section"
---@type string
LocationSection.FullID = ""

---Read/write current highlight state of the section. One of the `Highlight` constants.
---Since PopTracker 0.32.0.
---@type highlight
LocationSection.Highlight = 0
