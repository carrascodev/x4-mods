# X4 Mods - Nakama Multiplayer Integration

A collection of X4: Foundations mods featuring multiplayer capabilities through Nakama server integration. Built with a modular C++ architecture using shared commons library.

## 🏗️ Project Structure

```
x4-mods/
├── Mods/
│   ├── HenMod.Commons/          # Shared C++ utilities and base classes
│   └── NakamaX4Client/          # Nakama multiplayer mod for X4
├── build/                       # Build artifacts (generated)
├── vcpkg/                       # Package manager for C++ dependencies
└── build_mods.ps1            # PowerShell build script
```

## 🚀 Quick Start

### Prerequisites
- **X4: Foundations** with mod support
- **Nakama Server** running locally (default: localhost:7350)
- **Visual Studio 2022** with C++ development tools
- **PowerShell** for build scripts

### Build All Mods
```powershell
# Clean build and test everything
.\build_mods.ps1 -Mod all -BuildType Debug -Clean -Test -CopyFiles
```

### Build Individual Mods
```powershell
# Build only commons library
.\build_mods.ps1 -Mod commons -BuildType Release

# Build only NakamaX4Client
.\build_mods.ps1 -Mod nakama -BuildType Debug -Test
```

## 📦 Mods Included

### HenMod.Commons
Shared C++ library providing:
- Base classes (`X4ScriptBase`, `X4ScriptSingleton`)
- Logging utilities (`LogInfo`, `LogWarning`, `LogError`)
- Lua integration helpers
- Common dependencies (nlohmann_json, msgpack-cxx)

### NakamaX4Client
Multiplayer mod featuring:
- Real-time multiplayer via Nakama server
- Player authentication and data synchronization
- Sector-based matchmaking
- Lua bindings for game integration

## 🛠️ Development

### Build Options
- `-Mod`: `all`, `commons`, or `nakama`
- `-BuildType`: `Debug` (default) or `Release`
- `-Clean`: Remove build directory
- `-Test`: Build and run unit tests
- `-CopyFiles`: Copy DLLs to mod folders

### Testing
- **Catch2** for unit test framework
- **FakeIt** for dependency mocking
- Run tests: `.\build_mods.ps1 -Test`

### Debugging
1. Launch X4: `.\debug-x4.bat`
2. Attach debugger using VS Code "Launch X4 Debug" task
3. Monitor logs: `.\monitor-x4-logs.ps1`

## 🔧 Dependencies

### Vcpkg Packages
- `nlohmann-json` - JSON parsing
- `msgpack-cxx` - MessagePack serialization
- `catch2` - Unit testing framework
- `fakeit` - Mocking library

### Python Packages
- `requests` - HTTP client for build scripts
- `pywin32` - Windows API access

## 📚 Documentation

- [Copilot Instructions](.github/copilot-instructions.md) - AI assistant guidelines
- [NakamaX4Client README](Mods/NakamaX4Client/README.md) - Detailed mod documentation
- [Build Script Help](build_mods.ps1) - Run with `-?` for options

## 🤝 Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Add tests for new functionality
5. Ensure builds pass: `.\build_mods.ps1 -Test`
6. Submit a pull request

## 📄 License

See [LICENSE](LICENSE) file for details.

## 🙏 Credits

- **Heroic Labs** - Nakama server
- **Sir Nukes** - X4 mod support APIs
- **X4 Community** - Game modding support</content>
<parameter name="filePath">d:\dev\x4\README.md
