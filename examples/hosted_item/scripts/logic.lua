function SomeRule()
    print("Testing some rule")
    return Tracker:ProviderCountForCode("c") -- this is a number, not boolean!
    -- you can do arithmetic:
    -- addition `+` of two codes is logical Or
    -- multiplication `*` of two codes is logical And
end
