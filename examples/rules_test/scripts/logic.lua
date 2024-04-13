function SomeRule()
    print("Testing some rule")
    return Tracker:ProviderCountForCode("c") -- this is a number, not boolean!
    -- you can do arithmetic:
    -- addition `+` of two codes is logical Or
    -- multiplication `*` of two codes is logical And
end

function TrueIfB()
    local b = Tracker:FindObjectForCode("@B")
    ---@cast b Location
    return b.AccessibilityLevel > 0
end

function ToggleCodeA()
    local a = Tracker:FindObjectForCode("a")
    ---@cast a JsonItem
    a.Active = not a.Active
    return true
end
