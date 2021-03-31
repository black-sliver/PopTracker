# PopTracker Auto-Tracking

## Atomicity

Atomicity depends on backend and bridge.
Updating internal cache is atomic per block, but block size is not guaranteed.
Read block or u16, u24, u32, etc. from cache is atomic.

If you find a non-working case, please report.


## Interface

### global ScriptHost
* `:AddMemoryWatch(name, addr, size, callback[, interval_in_ms])`
* `:RemoveMemoryWatch(name)`
* callback signature:
`func(Segment)`


### global AutoTracker
`int :ReadU8(baseaddr,offset)` may return 0 if not yet cached. This may be slow.

### type Segment
`int :ReadUInt8(address)` may return 0 if not yet cached. This is better.


## Example

```
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
