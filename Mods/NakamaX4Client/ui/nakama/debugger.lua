-- Start debugger early (before other imports)
local success, err = pcall(function()
    package.cpath = package.cpath .. ";c:/Users/henri/.vscode/extensions/tangzx.emmylua-0.9.30-win32-x64/debugger/emmy/windows/x64/?.dll"
    local dbg = require("emmy_core")
    dbg.tcpListen("localhost", 9966)
    Register_Require_Response("extensions.NakamaX4Client.ui.nakama.debugger", dbg)
    return dbg
end)

if not success then
    -- Suppress the error for temporary script
    -- EmmyLua debugger not available: likely not in X4 environment
    Register_Require_Response("extensions.NakamaX4Client.ui.nakama.debugger", {})
end