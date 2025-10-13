#include "../public/lua_bindings.h"

// Global Lua state for callbacks - shared across all generated modules
lua_State *g_luaState = nullptr;