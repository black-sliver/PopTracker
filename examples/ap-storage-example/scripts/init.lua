-- UAT example pack by black_sliver
-- init.lua

Tracker:AddItems("items/items.json")
Tracker:AddLayouts("layouts/standard.json")

if Archipelago then
    ScriptHost:LoadScript("scripts/autotracking.lua")
end
