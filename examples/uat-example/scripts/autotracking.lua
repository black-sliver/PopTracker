-- UAT example pack by black_sliver
-- autotracking.lua

-- For this demo we named the item codes identical to the game variables.
-- Note that codes and variable names are case sensitive.
--
-- The return value of :ReadVariable can be anything, so we check the type and
-- * for toggles accept nil, false, integers <= 0 and empty strings as `false`
-- * for consumables everything that is not a number will be 0
-- * for progressive toggles we expect json [bool,number] or [number,number]
-- Alternatively try-catch (pcall) can be used to handle unexpected values.

function updateToggles(store, vars)
    print("updateToggles")
    for _, var in ipairs(vars) do
        local o = Tracker:FindObjectForCode(var)
        local val = store:ReadVariable(var)
        if type(val) == "number" then; o.Active = val > 0
        elseif type(val) == "string" then; o.Active = val ~= ""
        else; o.Active = not(not val)
        end
        print(var .. " = " .. tostring(val) .. " -> " .. tostring(o.Active))
    end
end

function updateConsumables(store, vars)
    print("updateConsumables")
    for _, var in ipairs(vars) do
        local o = Tracker:FindObjectForCode(var)
        local val = store:ReadVariable(var)
        if type(val) == "number" then; o.AcquiredCount = val
        else; o.AcquiredCount = 0
        end
        print(var .. " = " .. tostring(val) .. " -> " .. o.AcquiredCount)
    end
end

function updateProgressiveToggles(store, vars)
    print("updateProgressiveToggles")
    for _, var in ipairs(vars) do
        local o = Tracker:FindObjectForCode(var)
        local val = store:ReadVariable(var)
        if type(val) == "table" and type(val[2]) == "number" then
            if type(val[1]) == "number" then; o.Active = val[1]>0
            else; o.Active = not(not val[1])
            end
            o.CurrentStage = val[2]
        else
            o.Active = false
        end
        print(var .. " = " .. tostring(val) .. " -> " .. tostring(o.Active) .. "," .. o.CurrentStage)
    end
end

ScriptHost:AddVariableWatch("toggles", {"a"}, updateToggles)
ScriptHost:AddVariableWatch("consumables", {"b"}, updateConsumables)
ScriptHost:AddVariableWatch("progressive", {"c"}, updateProgressiveToggles)
