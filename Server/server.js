const fs = require("fs");

const config = JSON.parse(fs.readFileSync("config.json"));

const WebSocket = require('ws');


// --------------------------- SERVER FUNCTION ----------------------------------//

const PORT = 35992;

const wss = new WebSocket.Server({ port: PORT });

console.log(`WebSocket Server running on ws://localhost:${PORT}`);

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

// Handle server shutdown
process.on('SIGINT', () => {
    console.log('\n[SERVER] Shutting down...');
    wss.close(() => {

    });
    console.log('[SERVER] Server closed');
    process.exit(0);
});

function broadcastToClient(event) {
    console.log("Broadcasting " + JSON.stringify(event));
    const json = JSON.stringify(event);
    clients.forEach(client => {
        if (client.readyState === WebSocket.OPEN) {
            client.send(json);
        }
    });
}

//--------------------------- INDOFINITY CONNECTOR ---------------------------------//
const ws = new WebSocket('ws://localhost:62024');

// When the connection is successfully opened
ws.onopen = function () {
    console.log('Connected to IndoFinity WebSocket');
};

// When a message is received from the WebSocket
ws.onmessage = function (data) {
    try {
        const message = JSON.parse(data.data);
        const { event, data: eventData } = message; 

        //console.log(`Event: ${event}`);
        //console.log('Data:', eventData);

        // Example of handling chat events
        if (event === 'chat') {
            if(config.JoinUsingComment.enable) {
                if(eventData.comment == config.JoinUsingComment.comment) {
                    const eventSent = {
                        type: 'gift',
                        data: {
                            giftId: "c_join",
                            giftName: "Chat Join",
                            username: eventData.nickname,
                            userId: eventData.uniqueId,
                            quantity: 1,
                            timestamp: Date.now(),
                            diamondCost: 1
                        }
                    };
                    console.log(`@${eventData.uniqueId}: ${eventData.comment}`);
                    broadcastToClient(eventSent);
                }
            }
        } else if (event === "gift") {
            if(eventData.repeatEnd) return;
            const eventSent = {
                type: 'gift',
                data: {
                    giftId: eventData.giftId.toString(),
                    giftName: eventData.giftName,
                    username: eventData.nickname,
                    userId: eventData.uniqueId.toString(),
                    quantity: 1,
                    timestamp: Date.now(),
                    diamondCost: 1
                }
            };
            broadcastToClient(eventSent);
        }
    } catch (error) {
        console.error('Error parsing message:', error);
    }
};

// When WebSocket connection is closed
ws.onclose = function () {
    console.log('Koneksi WebSocket ditutup');
};

// When an error occurs in WebSocket
ws.onerror = function (err) {
    console.error('WebSocket error:', err);
};

// --------------------------------- GAME FUNCTION ----------------------------------//
/*
{
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
*/
