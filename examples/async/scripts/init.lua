-- UAT example pack by black_sliver
-- init.lua

Tracker:AddItems("items/items.json")
Tracker:AddLayouts("layouts/standard.json")

ScriptHost:RunScriptAsync("scripts/async.lua", "Hello, World!", function(result)
    print("Async result: " .. tostring(result))
end)
