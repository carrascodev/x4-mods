# NakamaX4Client for X4: Foundations

A high-performance mod that integrates X4: Foundations with Nakama server using a direct C++ DLL bridge for enhanced multiplayer features, leaderboards, and cloud saves.

## Features

- **Direct C++ Integration**: High-performance native DLL integration with X4's Lua environment
- **Device Authentication**: Secure authentication using device ID and username
- **Basic Data Synchronization**: Store player credits, playtime, and basic profile data
- **Leaderboard Support**: Submit scores to server-side leaderboards
- **Error Handling**: Comprehensive error reporting and status monitoring
- **Asynchronous Operations**: Non-blocking network calls with tick-based processing
- **Debug Support**: Detailed logging and status reporting for development

## Installation

1. Extract the mod to your X4 extensions folder:
   ```
   X4: Foundations/extensions/NakamaX4Client/
   ```

2. Install the required dependency: [sn_mod_support_apis](https://github.com/bvbohnen/x4-projects/tree/master/extensions/sn_mod_support_apis)

3. Build the C++ DLL bridge (see Build Instructions below)

4. Set up Nakama server (see Server Setup below)

## Architecture

```
X4 Game (Lua) <---> C++ DLL Bridge <---> HTTP/HTTPS <---> Nakama Server
```

The mod uses a native C++ DLL that provides Nakama SDK functions directly to X4's Lua environment, eliminating the need for external processes or Named Pipes.

## Build Instructions

### Requirements
- CMake 3.15+
- Visual Studio 2022 (or compatible C++20 compiler)
- vcpkg (for dependency management)
- Git

### Step 1: Install Dependencies

```cmd
# Install vcpkg if not already installed
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
.\bootstrap-vcpkg.bat

# Install required packages
.\vcpkg install nlohmann-json:x64-windows
```

### Step 2: Configure CMake

```cmd
cd cpp
mkdir build && cd build

# Configure with vcpkg toolchain
cmake -S .. -B . -DCMAKE_TOOLCHAIN_FILE="path/to/vcpkg/scripts/buildsystems/vcpkg.cmake"
```

### Step 3: Build the DLL

```cmd
# Build debug version (recommended - matches Nakama SDK)
cmake --build . --config Debug

# Output: build/Debug/nakama_x4.dll
```

### Step 4: Copy DLL to X4

```cmd
# Copy the built DLL to your X4 mod folder
copy build\Debug\nakama_x4.dll "path\to\X4\extensions\NakamaX4Client\ui\nakama\"
copy build\Debug\nakama-sdk.dll "path\to\X4\extensions\NakamaX4Client\ui\nakama\"
```

## Nakama Server Setup

### Docker Setup with Lua Support

Create a `docker-compose.yml` file:

```yaml
version: '3'
services:
  nakama:
    image: heroiclabs/nakama:3.18.0
    entrypoint:
      - "/bin/sh"
      - "-ecx"
      - >
        /nakama/nakama migrate up --database.address postgres:localdb@postgres:5432/nakama &&
        exec /nakama/nakama --name nakama1 --database.address postgres:localdb@postgres:5432/nakama --runtime.lua_entrypoint=/nakama/data/main.lua
    restart: "unless-stopped"
    links:
      - "postgres:db"
    depends_on:
      - postgres
    volumes:
      - ./server:/nakama/data
    ports:
      - "7349:7349"  # gRPC API
      - "7350:7350"  # HTTP API  
      - "7351:7351"  # Console
    environment:
      - NAKAMA_LOGGER_LEVEL=DEBUG
  postgres:
    command: postgres -c shared_preload_libraries=pg_stat_statements -c pg_stat_statements.track=all
    environment:
      - POSTGRES_DB=nakama
      - POSTGRES_PASSWORD=localdb
      - POSTGRES_USER=postgres
    image: postgres:12.2-alpine
    ports:
      - "5432:5432"
    volumes:
      - data:/var/lib/postgresql/data

volumes:
  data:
```

### Lua Server Code

Create `server/main.lua` to enable custom server logic:

```lua
local nk = require("nakama")

-- Custom match handler for multiplayer features
local function match_join_attempt(context, dispatcher, tick, state, presence, metadata)
    local acceptuser = true
    return state, acceptuser
end

local function match_join(context, dispatcher, tick, state, presences)
    for _, presence in ipairs(presences) do
        local msg = {
            user_id = presence.user_id,
            username = presence.username,
            op_code = 1
        }
        dispatcher.broadcast_message(2, nk.json_encode(msg))
    end
    return state
end

local function match_leave(context, dispatcher, tick, state, presences)
    for _, presence in ipairs(presences) do
        local msg = {
            user_id = presence.user_id,
            username = presence.username,
            op_code = 2
        }
        dispatcher.broadcast_message(3, nk.json_encode(msg))
    end
    return state
end

local function match_loop(context, dispatcher, tick, state, messages)
    -- Handle real-time messages from X4 clients
    for _, message in ipairs(messages) do
        local decoded = nk.json_decode(message.data)
        -- Broadcast X4 game state updates
        dispatcher.broadcast_message(message.op_code, message.data, {message.sender})
    end
    return state
end

-- Register match handler
nk.register_matchmaker_matched(function(context, matched_users)
    return nk.match_create("x4_match", {
        match_join_attempt = match_join_attempt,
        match_join = match_join,
        match_leave = match_leave,
        match_loop = match_loop,
        match_terminate = function() return nil end
    })
end)

-- Custom RPC for X4-specific operations
local function sync_player_data(context, payload)
    local user_id = context.user_id
    local data = nk.json_decode(payload)
    
    -- Store player data
    local objects = {
        {
            collection = "x4_player_data",
            key = "profile",
            user_id = user_id,
            value = data
        }
    }
    
    nk.storage_write(objects)
    
    return nk.json_encode({success = true})
end

nk.register_rpc(sync_player_data, "sync_player_data")

nk.logger_info("X4 Nakama server initialized with Lua support")
```

### Start the Server

```bash
# Create server directory if it doesn't exist
mkdir -p server

# Start with Docker Compose
docker-compose up -d

# Check logs
docker-compose logs nakama
```

## Usage

### Starting the System

1. **Start Nakama server**: `docker-compose up -d`
2. **Start X4** with the mod loaded
3. **Load a save game** - the mod will automatically:
   - Load the nakama_x4.dll
   - Call initialization manually from Lua code
   - Authenticate using device ID and username
   - Enable manual data synchronization calls

### Current Implementation

- **Manual Integration**: Functions must be called explicitly from X4 Lua code
- **Basic Data Sync**: Syncs credits and playtime to Nakama storage
- **Leaderboard Submission**: Submit scores to named leaderboards
- **Error Handling**: Get detailed error messages and status information
- **Tick Processing**: Call `nakama_tick()` regularly for async operation completion

### Available Lua Functions

The C++ DLL exposes these functions to X4's Lua environment:

```lua
-- Initialize connection to Nakama server
local result = nakama.nakama_init(host, port, server_key)

-- Authenticate with persistent device ID  
local result = nakama.nakama_authenticate(device_id, username)

-- Check authentication status
local is_auth = nakama.nakama_is_authenticated()

-- Sync basic player data to server
local result = nakama.nakama_sync_player_data(player_name, credits, playtime)

-- Submit score to leaderboard
local result = nakama.nakama_submit_score(leaderboard_id, score)

-- Get last error message
local error = nakama.nakama_get_last_error()

-- Get connection status
local status = nakama.nakama_get_status()

-- Process pending network operations (call regularly)
nakama.nakama_tick()

-- Shutdown connection
nakama.nakama_shutdown()
```

**Note**: All functions return 1 for success, 0 for failure (except getter functions).

## Communication Flow

1. **X4 mod** calls nakama functions directly via loaded DLL
2. **C++ DLL** uses Nakama C++ SDK to make HTTP/HTTPS requests
3. **Nakama server** processes requests using Lua server logic
4. **Nakama server** sends responses back to C++ DLL
5. **C++ DLL** returns results to X4 Lua environment

### Data Format

Player data is stored as JSON in Nakama storage with this simple structure:

```json
{
  "credits": 1000000,
  "playtime": 86400,
  "last_update": 1698765432
}
```

**Current Implementation**: 
- Basic player data sync (credits, playtime)
- Leaderboard score submission
- Device-based authentication
- Storage uses collection "player_data" with player name as key

## Configuration

### Nakama Connection Settings

Initialize the connection in your X4 Lua code:

```lua
-- Load the DLL and get the nakama functions
local nakama = require("extensions.NakamaX4Client.lua.nakama_lib")

if nakama then
    -- Initialize connection
    local host = "localhost"
    local port = 7350
    local server_key = "defaultkey"
    
    local result = nakama.nakama_init(host, port, server_key)
    if result == 1 then
        -- Authenticate
        local device_id = "unique_device_id_here"
        local username = "player_name"
        
        local auth_result = nakama.nakama_authenticate(device_id, username)
        if auth_result == 1 then
            -- Successfully connected and authenticated
            DebugError("[Nakama] Ready for data sync")
        end
    end
end
```

### Debug Mode

Enable debug logging in X4:
```
X4.exe -debug scripts -logfile nakama.log
```

## Troubleshooting

### Common Issues

1. **DLL loading failed**:
   - Ensure `nakama_x4.dll` and `nakama-sdk.dll` are in the mod's `ui/nakama/` folder
   - Check that you built the debug version (release version may have runtime conflicts)
   - Verify X4 can access the DLL files (no permission issues)

2. **Authentication failures**:
   - Verify Nakama server is running and accessible
   - Check Nakama server key matches in both client and server
   - Ensure device ID generation is working properly

3. **Connection issues**:
   - Check firewall settings for ports 7350, 7349, 7351
   - Verify Nakama server logs: `docker-compose logs nakama`
   - Test server accessibility: `curl http://localhost:7350`

### Debug Mode

1. **Enable X4 debug logging**:
   ```
   X4.exe -debug scripts -logfile nakama.log
   ```

2. **Check Nakama server logs**:
   ```bash
   docker-compose logs nakama
   ```

3. **Test DLL loading**:
   - Use the provided test suite in `/test` folder
   - Check that symbolic link to X4 extensions folder is working

## File Structure

```
NakamaX4Client/
├── content.xml              # Mod metadata
├── ui.xml                   # UI integration
├── cpp/                     # C++ source code
│   ├── src/                 # DLL source files
│   ├── include/             # Header files
│   ├── third-party/         # Nakama SDK
│   └── CMakeLists.txt       # Build configuration
├── ui/
│   └── nakama/
│       ├── nakama_lib.lua   # DLL loader
│       ├── nakama_client.lua # Client interface
│       └── nakama_x4.dll    # Built DLL (after build)
├── test/                    # Test suite
│   ├── test_nakama_dll.cpp  # C++ unit test
│   └── test_*.lua           # Lua integration tests
├── server/                  # Nakama server Lua code
│   └── main.lua             # Server logic
└── README.md                # This file
```

## Development

### Building from Source

```bash
# Clone the repository
git clone <repository-url>
cd NakamaX4Client

# Build using CMake
cd cpp
mkdir build && cd build
cmake -S .. -B . -DCMAKE_TOOLCHAIN_FILE="path/to/vcpkg/scripts/buildsystems/vcpkg.cmake"
cmake --build . --config Debug
```

### Testing

```bash
# Run C++ unit tests
cd test
./test_nakama_dll.exe

# Run Lua integration tests  
lua test_nakama_integration.lua
```

### Adding New Features

1. **C++ DLL Functions**: 
   - Add function declaration to `include/nakama_x4_api.h`
   - Implement function in `src/nakama_x4_dll.cpp`
   - Add Lua wrapper function (e.g., `lua_nakama_new_function`)
   - Register in `luaopen_nakama_x4` function table
2. **Lua Client Functions**: Extend `nakama_client.lua` to use new DLL functions
3. **Server Logic**: Add custom RPCs or match handlers to `server/main.lua`
4. **Testing**: Add corresponding tests to `/test` folder

### Current Limitations

- **No automatic sync**: All operations must be triggered manually from Lua
- **Basic data only**: Currently syncs credits and playtime only
- **No match/multiplayer**: Real-time multiplayer features require additional implementation
- **No leaderboard retrieval**: Can submit scores but not retrieve leaderboard data
- **Manual tick calls**: Requires regular `nakama_tick()` calls for async completion

### Performance Considerations

- All network operations are asynchronous to prevent blocking X4
- Call `nakama.tick()` regularly but not every frame
- Use connection pooling and proper error handling
- Monitor memory usage with large data transfers

## Security Notes

- Change default Nakama server key in production: `"defaultkey"` → `"your-secure-key"`
- Use HTTPS for production Nakama servers
- Implement proper authentication and authorization
- Validate all data before sending to Nakama server
- Consider rate limiting for API calls

## License

This mod is provided as-is for educational and development purposes.

## Credits

- **sn_mod_support_apis**: bvbohnen - Essential X4 modding framework
- **Nakama**: Heroic Labs - Game server framework and C++ SDK
- **X4 Community**: Modding support and documentation
- **vcpkg**: Microsoft - C++ package management