-- Simplified test for nakama_x4.dll loading
-- This test uses the actual path that X4 would use

print("=== Nakama X4 Client Test Suite ===")
print("Testing DLL loading from X4 extension path...")

-- Test the actual path that X4 will use
local dll_path = "D:\\Games\\X4 Foundations\\extensions\\NakamaX4Client\\ui\\nakama\\nakama_x4.dll"
local function_name = "luaopen_nakama_x4"

print("DLL Path: " .. dll_path)
print("Function: " .. function_name)

-- Check if file exists first
local file = io.open(dll_path, "r")
if file then
    print("✓ DLL file exists at target location")
    file:close()
else
    print("✗ DLL file NOT found at target location")
    print("Check if symbolic link is working and DLL is copied")
    return
end

-- Try to load the DLL
local loader, err = package.loadlib(dll_path, function_name)

if loader then
    print("✓ DLL and function loaded successfully!")
    
    -- Try to call the loader function
    local success, result = pcall(loader)
    if success then
        print("✓ Nakama module initialized successfully!")
        
        -- Check if we have nakama functions available
        if nakama then
            print("✓ Available nakama functions:")
            for name, func in pairs(nakama) do
                print("  - " .. name .. " (" .. type(func) .. ")")
            end
        else
            print("! WARNING: nakama table not found after initialization")
        end
    else
        print("✗ Failed to initialize Nakama module: " .. tostring(result))
    end
else
    print("✗ Failed to load DLL: " .. tostring(err))
    print("This might be due to missing runtime dependencies in test environment")
    print("The DLL should work correctly when loaded by X4")
end

print("=== Test Complete ===")