# GTA SA Battle Ground TikTok MOD

A GTA San Andreas MOD that integrates a battle royale system with TikTok Live streaming. Viewers can send gifts to spawn NPCs that fight each other, while the player acts as a spectator in free camera mode.

## Features

- **Free Camera Spectator Mode**: Watch battles from any angle with smooth camera controls
- **TikTok Live Integration**: WebSocket connection to receive gift events from TikTok Live
- **NPC Battle System**: NPCs fight each other using GTA SA's AI
- **One NPC Per Username**: Each viewer can only have one active NPC at a time
- **Revive System**: Dead NPCs can be revived using special gifts
- **Match System**: Automatic match management with countdown and winner declaration
- **Full HUD/UI**: Display match status, NPC health bars, kill feed, and notifications

## Requirements

- GTA San Andreas (PC Version 1.0)
- Windows 10/11
- CMake 3.30+
- Ninja build system
- C++17 compatible compiler (MSVC recommended)
- Boost libraries (system component)

## Installation

1. Build the project (see [SETUP.md](docs/SETUP.md))
2. Copy `GTASABattleGroundTikTok.asi` to your GTA SA game directory
3. Copy `config.ini` to your GTA SA game directory
4. Configure `config.ini` with your WebSocket server URL
5. Start GTA SA

## Controls

| Key | Action |
|-----|--------|
| WASD | Move camera |
| Mouse | Rotate camera |
| Space | Move camera up |
| Ctrl | Move camera down |
| Shift | Fast camera movement |
| Alt | Slow camera movement |
| F5 | Toggle free camera |
| F6 | Force start match |
| F7 | Reset match |
| F8 | Spawn test NPC (debug) |

## Configuration

See [CONFIGURATION.md](docs/CONFIGURATION.md) for detailed configuration options.

## WebSocket API

See [API.md](docs/API.md) for WebSocket message format and API documentation.

## How It Works

1. **Connect**: The mod connects to a WebSocket server that relays TikTok Live gift events
2. **Spawn**: When a viewer sends a gift, an NPC is spawned for them
3. **One Per User**: Each username can only have ONE alive NPC at a time
4. **Fight**: Once minimum NPCs are reached, the match starts and NPCs fight
5. **Revive**: Dead NPCs can be revived with special gift types
6. **Win**: Last NPC standing wins (after a delay period for revives)

## Success Criteria

- ✅ Player spawns in free camera mode
- ✅ WebSocket receives TikTok gifts
- ✅ NPCs spawn for each gift
- ✅ Each username can only have ONE alive NPC
- ✅ Spawn attempts blocked when username has alive NPC
- ✅ After NPC death, username can spawn again
- ✅ Match starts automatically at threshold
- ✅ NPCs fight using GTA SA AI
- ✅ Winner is determined correctly
- ✅ Dead NPCs can be revived with gifts
- ✅ 10-second delay before winner announcement
- ✅ All UI elements display correctly
- ✅ Configuration file works

## Project Structure

```
├── CMakeLists.txt
├── README.md
├── config.ini
├── docs/
│   ├── SETUP.md
│   ├── CONFIGURATION.md
│   └── API.md
├── src/
│   ├── main.cpp
│   ├── camera/
│   ├── config/
│   ├── match/
│   ├── network/
│   ├── npc/
│   ├── revive/
│   ├── ui/
│   └── utils/
├── libs/
│   ├── plugin-sdk/
│   └── json/
└── tests/
    ├── websocket_server_mock.js
    └── gift_simulator.py
```

## License

This project is for educational purposes only. GTA San Andreas is a trademark of Rockstar Games.

## Credits

- Plugin SDK by DK22Pac
- nlohmann/json for JSON parsing
