#pragma once
#include <string>

namespace LogToX4 {
	
	// Same as Log but without a Lua state (safe to call from non-Lua threads
	// as a best-effort log; still uses OutputDebugString and file append).
	void Log(const char* fmt, ...);

	// Utilities (exposed for tests or advanced usage)
	std::string GetModuleDir();                 // returns directory path of this DLL
	void AppendToModLog(const std::string& s); // append text to module log file
}