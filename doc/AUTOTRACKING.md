# PopTracker Auto-Tracking

## Supported Interfaces

* Memory
  * [USB2SNES websocket protocol](https://github.com/Skarsnik/QUsb2snes/blob/master/docs/Procotol.md)
    as provided by QUsb2Snes, Usb2Snes and SNI
* Variables
  * [UAT Protocol](https://github.com/black-sliver/UAT)
* Archipelago Multiworld
  * see [archipelago.gg](https://archipelago.gg)


## Atomicity

Atomicity depends on backend and bridge.
Updating internal cache is atomic per block, but block size is not guaranteed.
Read block or u16, u24, u32, etc. from cache is atomic.

If you find a non-working case, please report.


## Memory Interface (USB2SNES)

For snes, the inferface uses bus addresses, a valid address mapping has to be
provided, either through auto-detection (not implemented), variant's flags
"lorom", "hirom", "exlorom" or "exhirom", or the internal game-name table.


### global ScriptHost
* `:AddMemoryWatch(name, addr, size, callback[, interval_in_ms])` returns a reference (name) to the watch
* `:RemoveMemoryWatch(name)` removes named memory watch
* callback signature:
`function(Segment)` (see [type Segment](#type-segment))


### global AutoTracker
Reading from AutoTracker may be slower than reading from Segment (watch callback argument). `baseaddr`+`offset` is the absolute memory address.
* `int :ReadU8(baseaddr[,offset])` read 8bit unsigned integer, may return 0 if not yet cached
* `int :ReadU16(baseaddr[,offset])` as above, 16bit
* `int :ReadU24(baseaddr[,offset])` as above, 24bit
* `int :ReadU32(baseaddr[,offset])` as above, 32bit
* `table .SelectedConnectorType` currently is an empty table for compatibility

### type Segment
Reading from Segment (watch callback argument) is preferred. `address` is the absolute memory address.
* `int :ReadUInt8(address)` read 8bit unsigned integer, may return 0 if not yet cached
* `int :ReadUInt16(address)` as above, 16bit
* `int :ReadUInt24(address)` as above, 24bit
* `int :ReadUInt32(address)` as above, 32bit


### Example

```lua
-- init.lua

ScriptHost:LoadScript("scripts/autotracking.lua")


-- autotracking.lua

function updateAlchemy(mem)
    local b = mem:ReadUint8(0x7E2258)
    Tracker:FindObjectForCode("acid_rain").Active = (b & 0x01)>0 -- Acid Rain
    -- etc.
end


ScriptHost:AddMemoryWatch("AlchemyBlock", 0x7E2258, 5, updateAlchemy)
```


## Variable Interface (UAT)

To enable UAT, add the flag `"uat"` to your `manifest.json` -> `variants`
-> `<variant>` -> `flags`.

To auto-start [UATBridge](https://github.com/black-sliver/UATBridge),
if available, also add the flag `"uatbridge"`.


### global ScriptHost
* `:AddVariableWatch(name, {variable_name, ...}, callback[, interval_in_ms])` returns a reference (name) to the watch
* `:RemoveVariableWatch(name)`
* callback signature:
`function(Store, {changed_variable_name, ...})` (see [type Store](#type-store))


### global AutoTracker
Reading from AutoTracker may be slower than reading from Store (watch callback argument).
* `variant :ReadVariable(variable_name)` returns what the remote sent for the variable_name


### type Store
Reading from Store (watch callback argument) is preferred.
* `variant :ReadVariable(variable_name)` returns what the remote sent for the variable_name


### Example

```lua
-- init.lua

if AutoTracker.ReadVariable then
    ScriptHost:LoadScript("scripts/autotracking.lua")
end


-- autotracking.lua

function updateAlchemy(store)
    Tracker:FindObjectForCode("acid_rain").Active = store:ReadVariable("acid_rain")>0 -- Acid Rain
    -- etc.
end


ScriptHost:AddVariableWatch("Alchemy", {"acid_rain"}, updateAlchemy)
```

See [examples/uat-example](../examples/uat-example) for a full example.


## Archipelago Interface

This is fully callback-driven.

Archipelago auto-tracking is enabled by adding "ap" to a variant's flags in
manifest.json and clicking on "AP" in the menu when the pack is loaded.

* when connection to a server is established, Clear handlers are called so the
  Lua script can clear all locations and items.
* when an item is received, Item handlers are called
* when a location is checked, Location handlers are called
* when a location is scouted, Scout handlers are called
* when a bounce is received, Bounced handlers are called

### global Archipelago
* `.PlayerNumber` returns the slot number of the connected player or -1 if not connected
* `:AddClearHandler(name, callback)` called when connecting to a (new) server and state should be cleared; args: slot_data
* `:AddItemHandler(name, callback)` called when an item is received; args: index, item_id, item_name\[, player_number since 0.20.2\]
* `:AddLocationHandler(name, callback)` called when a location was checked; args: location_id, location_name
* `:AddScoutHandler(name, callback)` called when a location was scouted; args: location_id, location_name, item_id, item_name, item_player
* `:AddBouncedHandler(name, callback)` called when the server sends a bounce; args: json bounce message as table


## Other Stuff
in addition to above interfaces, the tracker will call
* `autotracker_started` when a game is connected (currently memory only)
* `autotracker_stopped` when a auto-tracking is disabled by the user (currently memory only)

in addition, the following back-end agnostic functions are provided
* `AutoTracker:GetConnectionState(backend_name)` will return an integer corresponding to the current state (since 0.20.2).
  * Valid backend names are: `"SNES"`, `"UAT"`, `"AP"` (case sensitive)
  * States are: -1: Unavailable, 0: Disabled, 1: Disconnected, 2: Socket Connected (not ready), 3: Game/Console Connected (ready)
