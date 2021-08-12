-- UAT example pack by black_sliver
-- init.lua

Tracker:AddItems("items/items.json")
Tracker:AddLayouts("layouts/standard.json")

if AutoTracker.ReadVariable then
    ScriptHost:LoadScript("scripts/autotracking.lua")
end
