#pragma once

#include <string>
#include <cstdarg>

extern "C" {
#include <lua.h>
}

namespace LogToX4 {
	// Log a formatted message and try to route it to Lua's DebugError if L is
	// provided. Always emits to OutputDebugString and appends to a persistent
	// module log file next to the DLL.
	void Log(lua_State* L, const char* fmt, ...);

	// Same as Log but without a Lua state (safe to call from non-Lua threads
	// as a best-effort log; still uses OutputDebugString and file append).
	void Log(const char* fmt, ...);

	// Utilities (exposed for tests or advanced usage)
	std::string GetModuleDir();                 // returns directory path of this DLL
	void AppendToModLog(const std::string& s); // append text to module log file
}

