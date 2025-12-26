#pragma once

#include "BattleNPC.h"
#include "../network/WebSocketClient.h"
#include "../utils/Math.h"

#include <vector>
#include <memory>
#include <unordered_map>
#include <string>
#include <functional>

#include <CDecisionMakerTypes.h>

namespace BattleGround {

// Spawn result for UI feedback
struct SpawnResult {
    bool success;
    std::string message;
    BattleNPC* npc;  // nullptr if failed
};

class NPCManager {
public:
    static NPCManager& GetInstance() {
        static NPCManager instance;
        return instance;
    }

    void Initialize();
    void Update(float deltaTime);
    void Shutdown();

    // NPC spawning with one-per-username rule
    SpawnResult SpawnNPC(const GiftEvent& gift);
    SpawnResult SpawnNPC(const std::string& username, const std::string& oderId, int weaponId = -1);
    
    // Check if username can spawn (doesn't have alive NPC)
    bool CanUsernameSpawn(const std::string& username) const;
    
    // NPC queries
    BattleNPC* GetNPCByUsername(const std::string& username);
    BattleNPC* GetNPCByPed(CPed* ped);
    std::vector<BattleNPC*> GetAllNPCs();
    std::vector<BattleNPC*> GetAliveNPCs();
    std::vector<BattleNPC*> GetDeadNPCs();
    
    // Stats
    int GetTotalNPCCount() const { return static_cast<int>(m_npcs.size()); }
    int GetAliveNPCCount() const;
    int GetDeadNPCCount() const;

    // NPC management
    void RemoveNPC(BattleNPC* npc);
    void RemoveNPC(const std::string& username);
    void RemoveAllNPCs();
    void RemoveDeadNPCs();

    // Events
    using NPCDeathCallback = std::function<void(BattleNPC* victim, BattleNPC* killer)>;
    using NPCSpawnCallback = std::function<void(BattleNPC* npc)>;
    
    void SetDeathCallback(NPCDeathCallback callback) { m_deathCallback = callback; }
    void SetSpawnCallback(NPCSpawnCallback callback) { m_spawnCallback = callback; }

    // Combat
    void MakeAllNPCsEnemies();
    void EnableAllCombatAI();
    void DisableAllCombatAI();

    // Decision Maker
    void GenerateDecisionMaker();
    eDecisionMakerType GetDecisionMaker() { return decisionMakerHandle; };

    CPed* CreatePed(const Math::Vector3& position);
    Math::Vector3 GetRandomSpawnPosition();

private:
    NPCManager() = default;
    ~NPCManager() = default;
    NPCManager(const NPCManager&) = delete;
    NPCManager& operator=(const NPCManager&) = delete;
    int GetRandomWeapon();
    void ProcessDeaths();
    void UpdateUsernameMap();

    std::vector<std::unique_ptr<BattleNPC>> m_npcs;
    
    // Hash map for fast username lookup
    // Maps username to NPC pointer
    // Only contains ALIVE NPCs (for spawn blocking)
    std::unordered_map<std::string, BattleNPC*> m_usernameToAliveNPC;
    
    // Map username to any NPC (alive or dead)
    std::unordered_map<std::string, BattleNPC*> m_usernameToNPC;

    // Callbacks
    NPCDeathCallback m_deathCallback;
    NPCSpawnCallback m_spawnCallback;

    // Cached config
    Math::Vector3 m_spawnCenter;
    float m_spawnRadius;
    bool m_oneNPCPerUsername;
    std::vector<int> m_weaponList;
    bool m_useRandomWeapons;
    int m_defaultWeaponId;

    eDecisionMakerType decisionMakerHandle = eDecisionMakerType::UNKNOWN;

    bool m_initialized = false;
};

} // namespace BattleGround
