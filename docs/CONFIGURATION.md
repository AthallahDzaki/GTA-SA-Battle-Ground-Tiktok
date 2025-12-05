# Configuration Guide

The `config.ini` file controls all aspects of the GTA SA Battle Ground TikTok MOD. This document explains each configuration option.

## Configuration File Location

Place `config.ini` in the same directory as `GTASABattleGroundTikTok.asi` (your GTA SA game directory).

## Configuration Sections

### [Match]

Controls match behavior and timing.

| Option | Type | Default | Description |
|--------|------|---------|-------------|
| MinNPCsToStart | int | 2 | Minimum NPCs required to start a match |
| CountdownSeconds | int | 5 | Countdown duration before match starts |
| WinnerDelaySeconds | int | 10 | Wait time before declaring winner (allows revives) |
| AutoStartMatch | bool | true | Automatically start match when minimum reached |

### [Spawn]

Controls NPC spawning behavior and location.

| Option | Type | Default | Description |
|--------|------|---------|-------------|
| CenterX | float | 2488.0 | X coordinate of spawn area center |
| CenterY | float | -1666.0 | Y coordinate of spawn area center |
| CenterZ | float | 13.5 | Z coordinate of spawn area center |
| SpawnRadius | float | 50.0 | Radius around center for random spawning |
| MinHeight | float | 10.0 | Minimum height offset for spawning |
| MaxHeight | float | 20.0 | Maximum height offset for spawning |
| OneNPCPerUsername | bool | true | **CRITICAL**: Only allow one NPC per username |

**Note**: The `OneNPCPerUsername` setting is recommended to be `true`. When enabled:
- Each username can only have ONE alive NPC at a time
- Spawn attempts are blocked if username has an alive NPC
- After NPC death, username can spawn again

### [WebSocket]

Controls WebSocket connection to TikTok Live server.

| Option | Type | Default | Description |
|--------|------|---------|-------------|
| ServerURL | string | ws://localhost:8080 | WebSocket server URL |
| AutoReconnect | bool | true | Automatically reconnect on disconnect |
| ReconnectIntervalSeconds | int | 5 | Seconds between reconnection attempts |
| ConnectionTimeout | int | 10 | Connection timeout in seconds |

### [Revive]

Controls NPC revival system.

| Option | Type | Default | Description |
|--------|------|---------|-------------|
| ReviveGiftIds | list | diamond,rose_gold,galaxy | Gift IDs that can revive NPCs |
| ReviveHealthPercent | int | 50 | Health percentage after revive (0-100) |
| InvulnerabilitySeconds | int | 3 | Invulnerability duration after revive |
| RequireOwnerMatch | bool | true | Only owner can revive their NPC |

### [Weapons]

Controls NPC weapon assignment.

| Option | Type | Default | Description |
|--------|------|---------|-------------|
| DefaultWeaponId | int | 22 | Default weapon (22 = 9mm pistol) |
| UseRandomWeapons | bool | true | Randomly select from weapon list |
| WeaponList | list | 22,24,25,28,29,30,31 | Available weapon IDs |

**Common Weapon IDs:**
- 22: 9mm Pistol
- 24: Desert Eagle
- 25: Shotgun
- 28: Micro SMG
- 29: MP5
- 30: AK-47
- 31: M4

### [Camera]

Controls free camera behavior.

| Option | Type | Default | Description |
|--------|------|---------|-------------|
| MoveSpeed | float | 1.0 | Base camera movement speed |
| FastSpeedMultiplier | float | 3.0 | Speed multiplier when holding Shift |
| SlowSpeedMultiplier | float | 0.3 | Speed multiplier when holding Ctrl |
| MouseSensitivity | float | 0.002 | Mouse sensitivity for rotation |
| AutoFollowEnabled | bool | false | Enable auto-follow for combat |
| AutoFollowDistance | float | 15.0 | Distance from target when following |

### [UI]

Controls HUD display options.

| Option | Type | Default | Description |
|--------|------|---------|-------------|
| ShowNPCNames | bool | true | Show username above NPC head |
| ShowHealthBars | bool | true | Show health bars above NPCs |
| ShowKillCount | bool | true | Show kill statistics |
| ShowMatchStatus | bool | true | Show match status overlay |
| NotificationDuration | float | 5.0 | Duration for notifications (seconds) |
| ShowGiftNotifications | bool | true | Show gift received notifications |

## Example Configuration

```ini
[Match]
MinNPCsToStart=5
CountdownSeconds=10
WinnerDelaySeconds=15
AutoStartMatch=true

[Spawn]
CenterX=2488.0
CenterY=-1666.0
CenterZ=13.5
SpawnRadius=100.0
MinHeight=10.0
MaxHeight=20.0
OneNPCPerUsername=true

[WebSocket]
ServerURL=ws://your-server.com:8080
AutoReconnect=true
ReconnectIntervalSeconds=5
ConnectionTimeout=10

[Revive]
ReviveGiftIds=diamond,rose_gold,galaxy
ReviveHealthPercent=50
InvulnerabilitySeconds=5
RequireOwnerMatch=true

[Weapons]
DefaultWeaponId=30
UseRandomWeapons=true
WeaponList=24,25,30,31

[Camera]
MoveSpeed=1.5
FastSpeedMultiplier=4.0
SlowSpeedMultiplier=0.2
MouseSensitivity=0.003

[UI]
ShowNPCNames=true
ShowHealthBars=true
ShowKillCount=true
ShowMatchStatus=true
NotificationDuration=5.0
ShowGiftNotifications=true
```

## Location Presets

### Los Santos (Default)
```ini
CenterX=2488.0
CenterY=-1666.0
CenterZ=13.5
```

### Las Venturas Strip
```ini
CenterX=2227.0
CenterY=1897.0
CenterZ=10.0
```

### San Fierro
```ini
CenterX=-2171.0
CenterY=252.0
CenterZ=35.0
```

### Grove Street
```ini
CenterX=2495.0
CenterY=-1668.0
CenterZ=13.3
```
