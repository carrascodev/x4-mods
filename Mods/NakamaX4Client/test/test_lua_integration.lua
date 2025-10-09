-- Test script to verify nakama_x4.dll loading outside of X4
print("Testing Lua integration with nakama_x4.dll...")

-- Try to load the DLL
local dll_path = "nakama_x4.dll"
local function_name = "luaopen_nakama_x4"

print("Loading DLL: " .. dll_path)
print("Looking for function: " .. function_name)

-- Use package.loadlib to load the DLL
local loader, err = package.loadlib(dll_path, function_name)

if loader then
    print("SUCCESS: DLL and function loaded successfully!")
    
    -- Try to call the loader function
    local success, result = pcall(loader)
    if success then
        print("SUCCESS: Nakama module initialized successfully!")
        
        -- Check if we have nakama functions available
        if nakama then
            print("Available nakama functions:")
            for name, func in pairs(nakama) do
                print("  " .. name .. " (" .. type(func) .. ")")
            end
        else
            print("WARNING: nakama table not found after initialization")
        end
    else
        print("ERROR: Failed to initialize nakama module: " .. tostring(result))
    end
else
    print("ERROR: Failed to load DLL function: " .. tostring(err))
end

print("Test completed.")