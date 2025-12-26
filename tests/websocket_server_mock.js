/**
 * WebSocket Server Mock for GTA SA Battle Ground TikTok MOD
 * 
 * This server simulates TikTok gift events for testing the mod.
 * 
 * Usage:
 *   node websocket_server_mock.js
 * 
 * The server will send simulated gift events every 5 seconds.
 */

const WebSocket = require('ws');

const PORT = 35992;
const GIFT_INTERVAL = 5000; // 5 seconds

// Available test gifts
const GIFTS = [
    { giftId: 'rose', giftName: 'Rose', diamondCost: 1 },
    { giftId: 'tiktok', giftName: 'TikTok', diamondCost: 1 },
    { giftId: 'finger_heart', giftName: 'Finger Heart', diamondCost: 5 },
    { giftId: 'panda', giftName: 'Panda', diamondCost: 5 },
    { giftId: 'diamond', giftName: 'Diamond', diamondCost: 100 },  // Revive gift
    { giftId: 'rose_gold', giftName: 'Rose Gold', diamondCost: 500 },  // Revive gift
    { giftId: 'galaxy', giftName: 'Galaxy', diamondCost: 1000 },  // Revive gift
];

// Test usernames pool
const USERNAMES = [
    'Viewer1',
    'TikTokFan',
    'GamerPro',
    'CoolUser123',
    'StreamWatcher',
    'NightOwl',
    'DayDreamer',
    'StarGazer',
];

// Track active usernames (for testing one-per-user rule)
const activeUsernames = new Set();

// WebSocket server
const wss = new WebSocket.Server({ port: PORT });

console.log(`WebSocket Mock Server running on ws://localhost:${PORT}`);
console.log('Sending gift events every 5 seconds...');
console.log('Press Ctrl+C to stop\n');

// Connected clients
const clients = new Set();

wss.on('connection', (ws) => {
    console.log('[SERVER] Client connected');
    clients.add(ws);

    ws.on('message', (message) => {
        console.log('[SERVER] Received:', message.toString());
    });

    ws.on('close', () => {
        console.log('[SERVER] Client disconnected');
        clients.delete(ws);
    });

    ws.on('error', (error) => {
        console.error('[SERVER] WebSocket error:', error);
    });

    // Send welcome message
    ws.send(JSON.stringify({
        type: 'info',
        data: { message: 'Connected to mock server' }
    }));
});

// Generate random gift event
function generateGiftEvent() {
    const gift = GIFTS[Math.floor(Math.random() * GIFTS.length)];
    const username = USERNAMES[Math.floor(Math.random() * USERNAMES.length)];
    const userId = Math.floor(Math.random() * 1000000).toString();

    return {
        type: 'gift',
        data: {
            giftId: gift.giftId,
            giftName: gift.giftName,
            username: username,
            userId: userId,
            quantity: 1,
            timestamp: Date.now(),
            diamondCost: gift.diamondCost
        }
    };
}

// Broadcast gift event to all clients
function broadcastGift() {
    const event = generateGiftEvent();
    const json = JSON.stringify(event);

    let sentCount = 0;
    clients.forEach((client) => {
        if (client.readyState === WebSocket.OPEN) {
            client.send(json);
            sentCount++;
        }
    });

    if (sentCount > 0) {
        console.log(`[GIFT] ${event.data.username} sent ${event.data.giftName} (${event.data.diamondCost} diamonds)`);
        
        // Track username for testing
        if (!activeUsernames.has(event.data.username)) {
            activeUsernames.add(event.data.username);
            console.log(`[INFO] New username: ${event.data.username} (total: ${activeUsernames.size})`);
        } else {
            console.log(`[INFO] ${event.data.username} already has an active NPC (spawn should be blocked)`);
        }
    }
}

// Send gift events periodically
//setInterval(broadcastGift, GIFT_INTERVAL);

// Handle server shutdown
process.on('SIGINT', () => {
    console.log('\n[SERVER] Shutting down...');
    wss.close(() => {
        console.log('[SERVER] Server closed');
        process.exit(0);
    });
});

// Also allow manual gift sending via command line
const readline = require('readline');
const rl = readline.createInterface({
    input: process.stdin,
    output: process.stdout
});

console.log('\nCommands:');
console.log('  g <username> - Send gift from specified username');
console.log('  r <username> - Send revive gift (diamond) from username');
console.log('  c            - Clear active usernames');
console.log('  l            - List active usernames');
console.log('  q            - Quit\n');

rl.on('line', (input) => {
    const parts = input.trim().split(' ');
    const cmd = parts[0].toLowerCase();

    switch (cmd) {
        case 'g':
            if (parts.length > 1) {
                const username = parts.slice(1).join(' ');
                const gift = GIFTS[Math.floor(Math.random() * 3)]; // Non-revive gift
                const event = {
                    type: 'gift',
                    data: {
                        giftId: gift.giftId,
                        giftName: gift.giftName,
                        username: username,
                        userId: '12345',
                        quantity: 1,
                        timestamp: Date.now(),
                        diamondCost: gift.diamondCost
                    }
                };
                const json = JSON.stringify(event);
                clients.forEach(client => {
                    if (client.readyState === WebSocket.OPEN) {
                        client.send(json);
                    }
                });
                console.log(`[MANUAL] Sent gift from ${username}`);
            }
            break;

        case 'r':
            if (parts.length > 1) {
                const username = parts.slice(1).join(' ');
                const event = {
                    type: 'gift',
                    data: {
                        giftId: 'diamond',
                        giftName: 'Diamond',
                        username: username,
                        userId: '12345',
                        quantity: 1,
                        timestamp: Date.now(),
                        diamondCost: 100
                    }
                };
                const json = JSON.stringify(event);
                clients.forEach(client => {
                    if (client.readyState === WebSocket.OPEN) {
                        client.send(json);
                    }
                });
                console.log(`[MANUAL] Sent revive gift from ${username}`);
            }
            break;

        case 'c':
            activeUsernames.clear();
            console.log('[INFO] Cleared active usernames');
            break;

        case 'l':
            console.log('[INFO] Active usernames:', Array.from(activeUsernames).join(', ') || 'none');
            break;

        case 'q':
            process.exit(0);
            break;

        default:
            console.log('Unknown command');
    }
});
