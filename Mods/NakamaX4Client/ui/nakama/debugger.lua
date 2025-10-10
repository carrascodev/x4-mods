-- Start debugger early (before other imports)
package.cpath = package.cpath ..
";c:/Users/henri/.vscode/extensions/tangzx.emmylua-0.9.29-win32-x64/debugger/emmy/windows/x64/?.dll"
local dbg = require("emmy_core")
dbg.tcpListen("localhost", 9966)


Register_Require_Response("extensions.NakamaX4Client.ui.nakama.debugger", dbg)
return dbg