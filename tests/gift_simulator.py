#!/usr/bin/env python3
"""
Gift Simulator for GTA SA Battle Ground TikTok MOD

This script connects to the WebSocket server and simulates gift events
for testing the mod's functionality.

Usage:
    python gift_simulator.py [--server ws://localhost:8080]

Features:
    - Simulates multiple users sending gifts
    - Tests one-NPC-per-username rule
    - Tests revive functionality
    - Interactive command mode
"""

import asyncio
import websockets
import json
import random
import time
import argparse
from datetime import datetime

# Configuration
DEFAULT_SERVER = "ws://localhost:35992"

# Test data
GIFTS = [
    {"giftId": "rose", "giftName": "Rose", "diamondCost": 1},
    {"giftId": "tiktok", "giftName": "TikTok", "diamondCost": 1},
    {"giftId": "finger_heart", "giftName": "Finger Heart", "diamondCost": 5},
    {"giftId": "panda", "giftName": "Panda", "diamondCost": 5},
]

REVIVE_GIFTS = [
    {"giftId": "diamond", "giftName": "Diamond", "diamondCost": 100},
    {"giftId": "rose_gold", "giftName": "Rose Gold", "diamondCost": 500},
    {"giftId": "galaxy", "giftName": "Galaxy", "diamondCost": 1000},
]

TEST_USERS = [
    "Alice",
    "Bob",
    "Charlie",
    "Diana",
    "Eve",
    "Frank",
]


class GiftSimulator:
    def __init__(self, server_url: str):
        self.server_url = server_url
        self.websocket = None
        self.active_users = set()
        self.dead_users = set()
        self.running = False

    async def connect(self):
        """Connect to WebSocket server."""
        print(f"Connecting to {self.server_url}...")
        try:
            self.websocket = await websockets.connect(self.server_url)
            print("Connected!")
            return True
        except Exception as e:
            print(f"Connection failed: {e}")
            return False

    async def disconnect(self):
        """Disconnect from WebSocket server."""
        if self.websocket:
            await self.websocket.close()
            print("Disconnected")

    async def send_gift(self, username: str, gift: dict) -> bool:
        """Send a gift event."""
        if not self.websocket:
            print("Not connected!")
            return False

        event = {
            "type": "gift",
            "data": {
                "giftId": gift["giftId"],
                "giftName": gift["giftName"],
                "username": username,
                "userId": str(hash(username) % 1000000),
                "quantity": 1,
                "timestamp": int(time.time() * 1000),
                "diamondCost": gift["diamondCost"]
            }
        }

        try:
            await self.websocket.send(json.dumps(event))
            print(f"[{datetime.now().strftime('%H:%M:%S')}] "
                  f"{username} sent {gift['giftName']} ({gift['diamondCost']} diamonds)")
            return True
        except Exception as e:
            print(f"Send failed: {e}")
            return False

    async def spawn_npc(self, username: str) -> bool:
        """Spawn an NPC for a user (non-revive gift)."""
        gift = random.choice(GIFTS)
        success = await self.send_gift(username, gift)
        if success:
            self.active_users.add(username)
        return success

    async def revive_npc(self, username: str) -> bool:
        """Send a revive gift for a user."""
        gift = random.choice(REVIVE_GIFTS)
        return await self.send_gift(username, gift)

    async def test_one_per_user(self):
        """Test that each username can only have one NPC."""
        print("\n=== Testing One NPC Per Username ===")
        
        user = "TestUser_OnePerUser"
        
        # First spawn should succeed
        print(f"\n1. First spawn for {user}:")
        await self.spawn_npc(user)
        await asyncio.sleep(1)
        
        # Second spawn should be blocked
        print(f"\n2. Second spawn for {user} (should be blocked):")
        await self.spawn_npc(user)
        await asyncio.sleep(1)
        
        # Third spawn should also be blocked
        print(f"\n3. Third spawn for {user} (should be blocked):")
        await self.spawn_npc(user)
        
        print("\n=== Test Complete ===\n")

    async def test_revive(self):
        """Test revive functionality."""
        print("\n=== Testing Revive System ===")
        
        user = "TestUser_Revive"
        
        # Spawn NPC
        print(f"\n1. Spawning NPC for {user}:")
        await self.spawn_npc(user)
        await asyncio.sleep(2)
        
        # Try revive (should fail - NPC is alive)
        print(f"\n2. Trying revive while NPC is alive (should fail):")
        await self.revive_npc(user)
        await asyncio.sleep(2)
        
        # NOTE: User needs to manually kill the NPC in game, then:
        print(f"\n3. After NPC dies, send revive gift to test:")
        print(f"   (Kill the NPC in game, then run: r {user})")
        
        print("\n=== Test Instructions Sent ===\n")

    async def auto_simulate(self, count: int = 10, interval: float = 3.0):
        """Automatically simulate gift events."""
        print(f"\n=== Auto Simulation: {count} gifts, {interval}s interval ===\n")
        
        for i in range(count):
            user = random.choice(TEST_USERS)
            
            # 80% chance of regular gift, 20% chance of revive gift
            if random.random() < 0.8:
                await self.spawn_npc(user)
            else:
                await self.revive_npc(user)
            
            await asyncio.sleep(interval)
        
        print("\n=== Auto Simulation Complete ===\n")

    async def interactive_mode(self):
        """Run in interactive command mode."""
        print("\nInteractive Mode - Commands:")
        print("  s <username>  - Spawn NPC for username")
        print("  r <username>  - Send revive gift for username")
        print("  a <count>     - Auto-simulate <count> gifts")
        print("  t1            - Test one-NPC-per-user rule")
        print("  t2            - Test revive system")
        print("  l             - List active users")
        print("  q             - Quit")
        print()

        self.running = True
        while self.running:
            try:
                cmd = await asyncio.get_event_loop().run_in_executor(
                    None, lambda: input("> ").strip()
                )
                
                if not cmd:
                    continue
                
                parts = cmd.split(maxsplit=1)
                action = parts[0].lower()
                
                if action == 'q':
                    self.running = False
                elif action == 's' and len(parts) > 1:
                    await self.spawn_npc(parts[1])
                elif action == 'r' and len(parts) > 1:
                    await self.revive_npc(parts[1])
                elif action == 'a':
                    count = int(parts[1]) if len(parts) > 1 else 10
                    await self.auto_simulate(count)
                elif action == 't1':
                    await self.test_one_per_user()
                elif action == 't2':
                    await self.test_revive()
                elif action == 'l':
                    print(f"Active users: {', '.join(self.active_users) or 'none'}")
                else:
                    print("Unknown command")
                    
            except EOFError:
                break
            except Exception as e:
                print(f"Error: {e}")


async def main():
    parser = argparse.ArgumentParser(description="Gift Simulator for GTA SA Battle Ground TikTok MOD")
    parser.add_argument("--server", default=DEFAULT_SERVER, help="WebSocket server URL")
    parser.add_argument("--auto", type=int, default=0, help="Auto-simulate N gifts then exit")
    args = parser.parse_args()

    simulator = GiftSimulator(args.server)
    
    if not await simulator.connect():
        return

    try:
        if args.auto > 0:
            await simulator.auto_simulate(args.auto)
        else:
            await simulator.interactive_mode()
    finally:
        await simulator.disconnect()


if __name__ == "__main__":
    asyncio.run(main())
