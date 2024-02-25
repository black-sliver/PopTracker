-- UAT example pack by black_sliver
-- init.lua

Tracker:AddMaps("maps/maps.json")
Tracker:AddLocations("locations/locations.json")
Tracker:AddItems("items/items.json")
Tracker:AddLayouts("layouts/standard.json")

if ScriptHost.AddVariableWatch then
    ScriptHost:LoadScript("scripts/autotracking.lua")
end
