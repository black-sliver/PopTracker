-- Zoom/Pan test pack by black_sliver
-- actions.lua

local a = Tracker:FindObjectForCode("a")
local b = Tracker:FindObjectForCode("b")
local c = Tracker:FindObjectForCode("c")

local MAP_WIDTH = 828
local MAP_HEIGHT = 400

local pan = function(x, y)
    -- NOTE: mapname is "map"
    local val = tostring(x) .. "," .. tostring(y)
    print("Panning to " .. val)
    Tracker:UiHint("Pan map", val)
end

local zoom = function(z)
    local val = tostring(z)
    print("Zooming to " .. val)
    Tracker:UiHint("Zoom map", val)
end

local reset = function()
    a.Active = false
    b.AcquiredCount = 0
    c.AcquiredCount = 0
    zoom(1)
    pan(MAP_WIDTH / 2, MAP_HEIGHT / 2)
end

local updatePan = function()
    local pos = b.AcquiredCount
    if pos > 0 then
        pos = pos - 1
        local xPos = pos % 10
        local yPos = pos // 10
        local x = MAP_WIDTH / 9 * xPos
        local y = MAP_HEIGHT / 9 * yPos
        pan(x, y)
    end
end

local updateZoom = function()
    local z = 1 + c.AcquiredCount * 1.25
    zoom(z)
end

ScriptHost:AddWatchForCode("reset", "a", reset)
ScriptHost:AddWatchForCode("updatePan", "b", updatePan)
ScriptHost:AddWatchForCode("updateZoom", "c", updateZoom)
