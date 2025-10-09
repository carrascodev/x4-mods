-- Simple test to check if DLL dependencies are satisfied
print("Testing nakama_x4.dll with X4's lua51_64.dll...")

-- Try to load the DLL - this will test if all dependencies are found
local dll_path = "nakama_x4.dll"
local function_name = "luaopen_nakama_x4"

print("Loading DLL: " .. dll_path)
print("Looking for function: " .. function_name)

-- Use package.loadlib to load the DLL
local loader, err = package.loadlib(dll_path, function_name)

if loader then
    print("SUCCESS: DLL loaded and function found!")
    print("This means all dependencies (including lua51_64.dll) are properly linked.")
else
    print("ERROR: " .. tostring(err))
end

print("Test completed.")