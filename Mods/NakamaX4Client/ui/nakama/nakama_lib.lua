--[[
Wrapper for loading the nakama_x4 dll file.

Following the pattern from sn_mod_support_apis winpipe.lua.
X4 would normally look for dll files only in "ui/core/lualibs/?_64.dll",
as defined in package.cpath. To load a dll from an extension folder, some
extra effort is needed using package.loadlib().

This is a wrapper that handles the loadlib() call for the nakama_x4 DLL.
]]

local L = nil

local function Load_Dll()
    local lib, err = package.loadlib(
        "D:\\Games\\X4 Foundations\\extensions\\NakamaX4Client\\ui\\nakama\\nakama_x4.dll", 
        "luaopen_nakama_x4")
    
    if lib then
        DebugError("[Nakama] DLL loaded successfully, calling luaopen_nakama_x4")
        return lib()  -- Call the function to get the table of functions
    else
        DebugError("[Nakama] DLL load failed: " .. tostring(err))
        return nil
    end
end

-- Check if this is running on Windows.
-- First character in package.config is the separator, which
-- is backslash on windows.
-- Note: dont try to load in ui protected mode.
if package ~= nil and package.config:sub(1,1) == "\\" and GetUISafeModeOption() == false then
    local success, value = pcall(Load_Dll, "")
    DebugError("[Nakama] DLL load attempt result: "..tostring(value))
    if success then
        L = value
    end
end

Register_Require_Response("extensions.NakamaX4Client.lua.nakama_lib", L)

return L