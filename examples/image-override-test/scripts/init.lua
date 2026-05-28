-- Image Override test pack by black_sliver
-- init.lua

Tracker:AddMaps("maps/maps.json")
Tracker:AddItems("items/items.json")
Tracker:AddLocations("locations/locations.json")
Tracker:AddLayouts("layouts/standard.json")

override_image = ImageReference:FromPackRelativePath("images/override.png")
disabled_override_image = ImageReference:FromImageReference(override_image, "@disabled")

---@type JsonItem
local json_item = Tracker:FindObjectForCode("json_item")
json_item.Icon = override_image

local lua_item = ScriptHost:CreateLuaItem()
lua_item.CanProvideCodeFunc = function(self, code) return code == "lua_item" end
lua_item.Name = "lua_item"
lua_item.Icon = override_image

local active = true
local timer = 0
function onFrame(elapsed)
    timer = timer + elapsed
    if timer > 0.5 then
        timer = timer - 0.5
        active = not active
        if active then
            json_item.Icon = override_image
            lua_item.Icon = override_image
        else
            json_item.Icon = disabled_override_image
            lua_item.Icon = disabled_override_image
        end
    end
end

ScriptHost:AddOnFrameHandler("onFrame", onFrame)
