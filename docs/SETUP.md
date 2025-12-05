# Development Setup Guide

This guide explains how to set up your development environment to build the GTA SA Battle Ground TikTok MOD.

## Prerequisites

### Required Software

1. **Visual Studio 2022** (or compatible MSVC compiler)
   - Download from: https://visualstudio.microsoft.com/
   - Install "Desktop development with C++" workload

2. **CMake 3.30+**
   - Download from: https://cmake.org/download/
   - Add to PATH during installation

3. **Ninja Build System**
   - Download from: https://ninja-build.org/
   - Or install via: `choco install ninja` (Chocolatey)

4. **Boost Libraries**
   - Download from: https://www.boost.org/
   - Build the `system` component
   - Set `BOOST_ROOT` environment variable

### Optional Tools

- **Git** for version control
- **Node.js** for running test WebSocket server
- **Python 3** for running gift simulator

## Building the Project

### 1. Clone the Repository

```bash
git clone --recursive https://github.com/AthallahDzaki/GTA-SA-Battle-Ground-Tiktok.git
cd GTA-SA-Battle-Ground-Tiktok
```

### 2. Initialize Submodules

```bash
git submodule update --init --recursive
```

### 3. Configure with CMake

```bash
mkdir build
cd build
cmake .. -G "Ninja" -DCMAKE_BUILD_TYPE=Release
```

### 4. Build

```bash
ninja
```

Or with CMake:

```bash
cmake --build . --config Release
```

### 5. Output Files

After successful build:
- `build/bin/GTASABattleGroundTikTok.asi` - The mod plugin
- `build/bin/config.ini` - Configuration file

## Installing the MOD

1. Copy `GTASABattleGroundTikTok.asi` to your GTA SA directory
2. Copy `config.ini` to your GTA SA directory
3. Make sure you have ASI Loader installed (Silent's ASI Loader recommended)

## Project Dependencies

### Plugin SDK

The project uses Plugin SDK for GTA SA modding:
- Included as git submodule in `libs/plugin-sdk/`
- Provides GTA SA game structures and functions

### nlohmann/json

Header-only JSON library:
- Located in `libs/json/include/json.hpp`
- Used for parsing WebSocket messages

### Boost

Required components:
- `Boost::system` - Used by WebSocket implementation

## Troubleshooting

### CMake can't find Boost

Set the `BOOST_ROOT` environment variable:
```bash
set BOOST_ROOT=C:\path\to\boost_1_xx_x
```

### Linker errors with plugin-sdk

Make sure to use MSVC compiler and that DirectX SDK is available.

### Game crashes on startup

1. Check BattleGround.log for errors
2. Verify config.ini is in the game directory
3. Make sure you're using GTA SA version 1.0

## IDE Setup

### Visual Studio Code

Recommended extensions:
- C/C++ (Microsoft)
- CMake Tools
- CMake

Configure `.vscode/settings.json`:
```json
{
    "cmake.generator": "Ninja",
    "cmake.configureOnOpen": true
}
```

### Visual Studio

1. Open folder containing CMakeLists.txt
2. VS will auto-detect CMake project
3. Select Release configuration
4. Build > Build All

## Testing

### Run Mock WebSocket Server

```bash
cd tests
node websocket_server_mock.js
```

### Run Gift Simulator

```bash
cd tests
python gift_simulator.py
```

## Development Notes

- The project uses C++17 standard
- All GTA SA interactions go through Plugin SDK
- One NPC per username is enforced in NPCManager
- WebSocket uses simple RFC 6455 implementation
