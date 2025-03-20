-- Read settings test pack by black_sliver
-- init.lua

local json = require("json")
local settings_file = io.open("settings.json", "rb")
local settings_data = settings_file:read("*all")
settings_file:close()
local settings = json.decode(settings_data)
print(settings["key"])  -- should print "value"
