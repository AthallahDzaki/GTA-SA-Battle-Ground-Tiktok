/**
 * GTA SA Battle Ground TikTok MOD
 * 
 * A Battle Royale MOD for GTA San Andreas that integrates with TikTok Live.
 * Viewers can send gifts to spawn NPCs that fight each other.
 * 
 * Features:
 * - Free camera spectator mode
 * - WebSocket integration for TikTok gifts
 * - NPC spawning with one-per-username rule
 * - Automatic match system
 * - Combat AI for NPCs
 * - Revive system
 * - Full HUD/UI
 */

#include <plugin.h>
#include <CTimer.h>
#include <CGame.h>
#include <CWorld.h>
#include <CCamera.h>

#include "game_sa/common.h"
#include "config/ConfigManager.h"
#include "utils/Logger.h"
#include "camera/FreeCameraController.h"
#include "network/WebSocketClient.h"
#include "npc/NPCManager.h"
#include "match/MatchController.h"
#include "match/CombatSystem.h"
#include "revive/ReviveSystem.h"
#include "ui/UIRenderer.h"

using namespace BattleGround;

class GTASABattleGroundTikTok {
public:
    GTASABattleGroundTikTok() {
        // Register game events
        plugin::Events::initRwEvent += []() {
            OnGameInit();
        };

        plugin::Events::shutdownRwEvent += []() {
            OnGameShutdown();
        };

        plugin::Events::gameProcessEvent += []() {
            OnGameUpdate();
        };

        plugin::Events::restartGameEvent += []{ 
            OnGameRestart(); 
        };

        plugin::Events::initGameEvent += []() {
            
            NPCManager::GetInstance().GenerateDecisionMaker();
            LOG_DEBUG("Decision Maker Generated");
        };

        plugin::Events::drawingEvent += []() {
            OnGameRender();
        };

        plugin::Events::drawHudEvent += []() {
            OnHudRender();
        };
    }

private:
    static bool s_initialized;
    static float s_lastTime;

    static void
    UnProtectInstance ()
    {
        auto              hExecutableInstance = (size_t) GetModuleHandle (NULL);
        IMAGE_NT_HEADERS *ntHeader
            = (IMAGE_NT_HEADERS *) (hExecutableInstance
                                    + ((IMAGE_DOS_HEADER *) hExecutableInstance)
                                          ->e_lfanew);
        SIZE_T size = ntHeader->OptionalHeader.SizeOfImage;
        DWORD  oldProtect;
        VirtualProtect ((VOID *) hExecutableInstance, size, PAGE_EXECUTE_READWRITE,
                        &oldProtect);
    }

    static void OnGameRestart() {
        NPCManager::GetInstance().GenerateDecisionMaker(); // Regenerate Decision Maker
    }

    static void OnGameInit() {
        // Initialize logger first
        Logger::GetInstance().Initialize("BattleGround.log");
        LOG_INFO("=== GTA SA Battle Ground TikTok MOD ===");
        LOG_INFO("Initializing...");

        // Load configuration
        if (!ConfigManager::GetInstance().Load("config.ini")) {
            LOG_WARNING("Failed to load config.ini, using defaults");
        }

        // Initialize all systems
        FreeCameraController::GetInstance().Initialize();
        WebSocketClient::GetInstance().Initialize();
        NPCManager::GetInstance().Initialize();
        MatchController::GetInstance().Initialize();
        CombatSystem::GetInstance().Initialize();
        ReviveSystem::GetInstance().Initialize();
        UIRenderer::GetInstance().Initialize();

        // Set up callbacks
        SetupCallbacks();

        // Connect to WebSocket server
        WebSocketClient::GetInstance().Connect();

        // Enable free camera
        //FreeCameraController::GetInstance().SetEnabled(true);

        UnProtectInstance();

        s_initialized = true;
        s_lastTime = static_cast<float>(CTimer::m_snTimeInMilliseconds) / 1000.0f;
        
        LOG_INFO("Initialization complete!");
    }

    static void OnGameShutdown() {
        if (!s_initialized) return;

        LOG_INFO("Shutting down...");

        // Shutdown all systems in reverse order
        UIRenderer::GetInstance().Shutdown();
        ReviveSystem::GetInstance().Shutdown();
        CombatSystem::GetInstance().Shutdown();
        MatchController::GetInstance().Shutdown();
        NPCManager::GetInstance().Shutdown();
        WebSocketClient::GetInstance().Shutdown();
        //FreeCameraController::GetInstance().Shutdown();

        Logger::GetInstance().Shutdown();
        s_initialized = false;
    }

    static void OnGameUpdate() {
        if (!s_initialized || !CGame::CanSeeOutSideFromCurrArea()) return;

        *reinterpret_cast<bool *> (0x96917A) = true;

        // Calculate delta time
        float currentTime = static_cast<float>(CTimer::m_snTimeInMilliseconds) / 1000.0f;
        float deltaTime = currentTime - s_lastTime;
        s_lastTime = currentTime;

        // Clamp delta time to prevent huge jumps
        if (deltaTime > 0.1f) deltaTime = 0.1f;

        // Handle input for free camera
        HandleInput();

        // Update all systems
        //FreeCameraController::GetInstance().Update(deltaTime);
        WebSocketClient::GetInstance().Update();
        NPCManager::GetInstance().Update(deltaTime);
        MatchController::GetInstance().Update(deltaTime);
        CombatSystem::GetInstance().Update(deltaTime);
        ReviveSystem::GetInstance().Update(deltaTime);
        UIRenderer::GetInstance().Update(deltaTime);

        // Process gift events
        ProcessGiftEvents();
    }

    static void OnGameRender() {
        if (!s_initialized) return;
        
        // 3D rendering (if needed)
    }

    static void OnHudRender() {
        if (!s_initialized) return;
        
        // Render UI
        UIRenderer::GetInstance().Render();
    }

    static void HandleInput() {
        // Get keyboard state for camera control
        bool forward = GetAsyncKeyState('W') & 0x8000;
        bool backward = GetAsyncKeyState('S') & 0x8000;
        bool left = GetAsyncKeyState('A') & 0x8000;
        bool right = GetAsyncKeyState('D') & 0x8000;
        bool up = GetAsyncKeyState(VK_SPACE) & 0x8000;
        bool down = GetAsyncKeyState(VK_CONTROL) & 0x8000;
        bool fast = GetAsyncKeyState(VK_SHIFT) & 0x8000;
        bool slow = GetAsyncKeyState(VK_MENU) & 0x8000; // Alt

        //FreeCameraController::GetInstance().HandleKeyboardInput(forward, backward, left, right, up, down, fast, slow);

        // Mouse movement for camera rotation
        static int lastMouseX = 0;
        static int lastMouseY = 0;
        
        POINT mousePos;
        GetCursorPos(&mousePos);
        
        float deltaX = static_cast<float>(mousePos.x - lastMouseX);
        float deltaY = static_cast<float>(mousePos.y - lastMouseY);
        
        lastMouseX = mousePos.x;
        lastMouseY = mousePos.y;
        
        //FreeCameraController::GetInstance().HandleMouseMovement(deltaX, deltaY);

        // Toggle camera with F5
        /*
        static bool f5Pressed = false;
        if (GetAsyncKeyState(VK_F5) & 0x8000) {
            if (!f5Pressed) {
                f5Pressed = true;
                bool enabled = !FreeCameraController::GetInstance().IsEnabled();
                FreeCameraController::GetInstance().SetEnabled(enabled);
                LOG_INFO(enabled ? "Free camera enabled" : "Free camera disabled");
            }
        } else {
            f5Pressed = false;
        }*/

        // Force start match with F6
        static bool f6Pressed = false;
        if (GetAsyncKeyState(VK_F6) & 0x8000) {
            if (!f6Pressed) {
                f6Pressed = true;
                MatchController::GetInstance().ForceStart();
            }
        } else {
            f6Pressed = false;
        }

        // Reset match with F7
        static bool f7Pressed = false;
        if (GetAsyncKeyState(VK_F7) & 0x8000) {
            if (!f7Pressed) {
                f7Pressed = true;
                MatchController::GetInstance().Reset();
                NPCManager::GetInstance().RemoveAllNPCs();
                UIRenderer::GetInstance().AddNotification(NotificationType::MATCH_END, "Match reset!");
            }
        } else {
            f7Pressed = false;
        }

        // Debug: Spawn test NPC with F8
        static bool f8Pressed = false;
        if (GetAsyncKeyState(VK_F8) & 0x8000) {
            if (!f8Pressed) {
                f8Pressed = true;
                static int testCounter = 0;
                std::string testUser = "TestUser" + std::to_string(++testCounter);
                auto result = NPCManager::GetInstance().SpawnNPC(testUser, "test123");
                if (result.success) {
                    UIRenderer::GetInstance().AddNotification(NotificationType::SPAWN_SUCCESS, 
                        "[TEST] " + testUser + " spawned!");
                } else {
                    UIRenderer::GetInstance().AddNotification(NotificationType::SPAWN_BLOCKED, 
                        result.message);
                }
            }
        } else {
            f8Pressed = false;
        }
    }

    static void ProcessGiftEvents() {
        GiftEvent event;
        while (WebSocketClient::GetInstance().PopEvent(event)) {
            LOG_INFO("Processing gift: " + event.giftName + " from " + event.username);

            // Check if it's a revive gift
            if (ReviveSystem::GetInstance().IsReviveGift(event.giftId)) {
                auto result = ReviveSystem::GetInstance().TryRevive(event);
                if (result.success) {
                    UIRenderer::GetInstance().AddNotification(NotificationType::REVIVE, result.message);
                } else {
                    // Not a revive situation, try spawning
                    TrySpawnFromGift(event);
                }
            } else {
                // Regular spawn gift
                TrySpawnFromGift(event);
            }
        }
    }

    static void TrySpawnFromGift(const GiftEvent& event) {
        // Spawn NPCs based on quantity
        for (int i = 0; i < event.quantity; i++) {
            auto result = NPCManager::GetInstance().SpawnNPC(event);
            
            if (result.success) {
                UIRenderer::GetInstance().AddNotification(NotificationType::GIFT_RECEIVED,
                    "[" + event.username + "] sent " + event.giftName + "! NPC spawned!");
                
                // Notify match controller
                MatchController::GetInstance().OnNPCCountChanged();
            } else {
                // Spawn was blocked (likely username already has NPC)
                UIRenderer::GetInstance().AddNotification(NotificationType::SPAWN_BLOCKED,
                    result.message + " ⚠️");
            }
            
            // Only one NPC per gift event per username
            break;
        }
    }

    static void SetupCallbacks() {
        // NPC death callback
        NPCManager::GetInstance().SetDeathCallback([](BattleNPC* victim, BattleNPC* killer) {
            if (victim) {
                if (killer) {
                    UIRenderer::GetInstance().AddKillFeedEntry(killer->GetUsername(), victim->GetUsername());
                    UIRenderer::GetInstance().AddNotification(NotificationType::NPC_DEATH,
                        killer->GetUsername() + " eliminated " + victim->GetUsername());
                } else {
                    UIRenderer::GetInstance().AddNotification(NotificationType::NPC_DEATH,
                        victim->GetUsername() + " died");
                }
            }
        });

        // NPC spawn callback
        NPCManager::GetInstance().SetSpawnCallback([](BattleNPC* npc) {
            if (npc) {
                LOG_DEBUG("NPC spawned callback: " + npc->GetUsername());
            }
        });

        // Match state change callback
        MatchController::GetInstance().SetStateChangeCallback([](MatchState oldState, MatchState newState) {
            switch (newState) {
                case MatchState::COUNTDOWN:
                    UIRenderer::GetInstance().AddNotification(NotificationType::MATCH_START,
                        "Match starting soon!");
                    break;
                case MatchState::IN_PROGRESS:
                    UIRenderer::GetInstance().AddNotification(NotificationType::MATCH_START,
                        "FIGHT!");
                    CombatSystem::GetInstance().EnableCombat();
                    break;
                case MatchState::FINISHED:
                    CombatSystem::GetInstance().DisableCombat();
                    break;
                default:
                    break;
            }
        });

        // Winner callback
        MatchController::GetInstance().SetWinnerCallback([](BattleNPC* winner) {
            if (winner) {
                UIRenderer::GetInstance().AddNotification(NotificationType::WINNER,
                    "WINNER: " + winner->GetUsername() + "!");
            }
        });

        // Revive callback
        ReviveSystem::GetInstance().SetReviveCallback([](BattleNPC* npc) {
            if (npc) {
                UIRenderer::GetInstance().AddNotification(NotificationType::REVIVE,
                    npc->GetUsername() + " has been revived!");
            }
        });

        // WebSocket connection callback
        WebSocketClient::GetInstance().SetConnectionCallback([](bool connected) {
            if (connected) {
                LOG_INFO("WebSocket connected!");
            } else {
                LOG_WARNING("WebSocket disconnected");
            }
        });
    }

} gtaSABattleGroundTikTok;

// Static member initialization
bool GTASABattleGroundTikTok::s_initialized = false;
float GTASABattleGroundTikTok::s_lastTime = 0;
