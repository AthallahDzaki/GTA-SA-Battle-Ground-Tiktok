# WebSocket API Documentation

This document describes the WebSocket API for communicating with the GTA SA Battle Ground TikTok MOD.

## Connection

The mod connects as a WebSocket client to the server specified in `config.ini`.

**Default URL**: `ws://localhost:8080`

### Connection Flow

1. Client initiates WebSocket handshake
2. Server accepts connection
3. Server sends gift events as JSON messages
4. Client processes events and spawns NPCs

## Message Format

### Gift Event

Sent from server to client when a TikTok gift is received.

```json
{
  "type": "gift",
  "data": {
    "giftId": "rose",
    "giftName": "Rose",
    "username": "viewer123",
    "userId": "12345",
    "quantity": 1,
    "timestamp": 1234567890,
    "diamondCost": 1
  }
}
```

#### Fields

| Field | Type | Description |
|-------|------|-------------|
| type | string | Must be "gift" |
| data.giftId | string | Unique gift identifier |
| data.giftName | string | Display name of the gift |
| data.username | string | TikTok username of sender |
| data.userId | string | TikTok user ID of sender |
| data.quantity | int | Number of gifts sent |
| data.timestamp | int | Unix timestamp (milliseconds) |
| data.diamondCost | int | Cost of gift in diamonds |

### Processing Rules

1. **One NPC Per Username**: Each username can only have ONE alive NPC
2. **Spawn Blocking**: If username has an alive NPC, spawn is rejected
3. **Revive Detection**: If giftId is in ReviveGiftIds, try revive first
4. **Quantity**: Only one NPC spawns per gift event (regardless of quantity)

## Revive Gifts

Special gift IDs that can revive dead NPCs:

- `diamond`
- `rose_gold`
- `galaxy`

Configure in `config.ini` under `[Revive] ReviveGiftIds`.

### Revive Logic

1. Check if gift is a revive gift
2. Check if sender has a dead NPC
3. Check if match is active (IN_PROGRESS or WINNER_DELAY)
4. If RequireOwnerMatch=true, only revive sender's NPC
5. Revive with configured health percentage
6. Apply temporary invulnerability

## Server Implementation

### Node.js Example

```javascript
const WebSocket = require('ws');

const wss = new WebSocket.Server({ port: 8080 });

wss.on('connection', (ws) => {
    console.log('Client connected');

    // Send test gift
    const gift = {
        type: 'gift',
        data: {
            giftId: 'rose',
            giftName: 'Rose',
            username: 'TestUser',
            userId: '123',
            quantity: 1,
            timestamp: Date.now(),
            diamondCost: 1
        }
    };
    
    ws.send(JSON.stringify(gift));
});

console.log('WebSocket server running on ws://localhost:8080');
```

### TikTok Integration

To integrate with TikTok Live:

1. Use TikTok Live API or third-party libraries
2. Convert gift events to the above JSON format
3. Forward to WebSocket clients

Example using TikTok-Live-Connector (Node.js):

```javascript
const { WebcastPushConnection } = require('tiktok-live-connector');
const WebSocket = require('ws');

// WebSocket server
const wss = new WebSocket.Server({ port: 8080 });
const clients = new Set();

wss.on('connection', (ws) => {
    clients.add(ws);
    ws.on('close', () => clients.delete(ws));
});

// TikTok connection
const tiktok = new WebcastPushConnection('@username');

tiktok.connect().then(() => {
    console.log('Connected to TikTok Live');
});

tiktok.on('gift', (data) => {
    const message = {
        type: 'gift',
        data: {
            giftId: data.giftId.toString(),
            giftName: data.giftName,
            username: data.uniqueId,
            userId: data.userId.toString(),
            quantity: data.repeatCount,
            timestamp: Date.now(),
            diamondCost: data.diamondCount
        }
    };
    
    const json = JSON.stringify(message);
    clients.forEach(client => {
        if (client.readyState === WebSocket.OPEN) {
            client.send(json);
        }
    });
});
```

## Error Handling

### Connection Errors

The mod will automatically reconnect if:
- `AutoReconnect=true` in config
- Connection is lost or fails

Reconnection interval: `ReconnectIntervalSeconds` (default: 5)

### Invalid Messages

Invalid JSON messages are logged but ignored. Check `BattleGround.log` for errors.

## Testing

Use the included test server:

```bash
cd tests
node websocket_server_mock.js
```

Use the gift simulator:

```bash
cd tests
python gift_simulator.py
```

## Events Summary

| Event | Direction | Description |
|-------|-----------|-------------|
| gift | Server→Client | TikTok gift received |

## Future Extensions

The API may be extended to support:
- `chat` - Chat messages
- `follow` - New followers
- `like` - Likes received
- `share` - Shares
- `subscribe` - Subscriptions

Check for updates in future versions.
