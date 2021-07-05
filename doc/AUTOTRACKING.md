# PopTracker Auto-Tracking

## Supported Interfaces

* Memory
  * [USB2SNES websocket protocol](https://github.com/Skarsnik/QUsb2snes/blob/master/docs/Procotol.md)
    as provided by QUsb2Snes, Usb2Snes and SNI
* Variables
  * [UAT Protocol](https://github.com/black-sliver/UAT)


## Atomicity

Atomicity depends on backend and bridge.
Updating internal cache is atomic per block, but block size is not guaranteed.
Read block or u16, u24, u32, etc. from cache is atomic.

If you find a non-working case, please report.


## Memory Interface (USB2SNES)

### global ScriptHost
* `:AddMemoryWatch(name, addr, size, callback[, interval_in_ms])`
* `:RemoveMemoryWatch(name)`
* callback signature:
`function(Segment)` (see [type Segment](#type-segment))


### global AutoTracker
Reading from AutoTracker may be slower than reading from Segment (watch callback argument). `baseaddr`+`offset` is the absolute memory address.
* `int :ReadU8(baseaddr[,offset])` read 8bit unsigned integer, may return 0 if not yet cached
* `int :ReadU16(baseaddr[,offset])` as above, 16bit
* `int :ReadU24(baseaddr[,offset])` as above, 24bit
* `int :ReadU32(baseaddr[,offset])` as above, 32bit

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
* `:AddVariableWatch(name, {variable_name, ...}, callback[, interval_in_ms])`
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
