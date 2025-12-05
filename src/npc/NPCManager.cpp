#include "NPCManager.h"
#include "../config/ConfigManager.h"
#include "../utils/Logger.h"

// GTA SA SDK includes
#include <CWorld.h>
#include <CPools.h>
#include <CPopulation.h>
#include <CPed.h>
#include <CPedModelInfo.h>
#include <CStreaming.h>
#include <CModelInfo.h>

namespace BattleGround {

void NPCManager::Initialize() {
    if (m_initialized) return;

    const auto& spawnConfig = ConfigManager::GetInstance().GetSpawnConfig();
    m_spawnCenter = Math::Vector3(spawnConfig.centerX, spawnConfig.centerY, spawnConfig.centerZ);
    m_spawnRadius = spawnConfig.spawnRadius;
    m_oneNPCPerUsername = spawnConfig.oneNPCPerUsername;

    const auto& weaponsConfig = ConfigManager::GetInstance().GetWeaponsConfig();
    m_weaponList = weaponsConfig.weaponList;
    m_useRandomWeapons = weaponsConfig.useRandomWeapons;
    m_defaultWeaponId = weaponsConfig.defaultWeaponId;

    m_initialized = true;
    LOG_INFO("NPC Manager initialized");
}

void NPCManager::Update(float deltaTime) {
    // Update all NPCs
    for (auto& npc : m_npcs) {
        if (npc) {
            npc->Update(deltaTime);
        }
    }

    // Check for deaths and update username maps
    ProcessDeaths();
    UpdateUsernameMap();
}

void NPCManager::Shutdown() {
    RemoveAllNPCs();
    m_initialized = false;
    LOG_INFO("NPC Manager shutdown");
}

bool NPCManager::CanUsernameSpawn(const std::string& username) const {
    if (!m_oneNPCPerUsername) {
        return true; // No restriction
    }
    
    // Check if username has an ALIVE NPC
    auto it = m_usernameToAliveNPC.find(username);
    return it == m_usernameToAliveNPC.end();
}

SpawnResult NPCManager::SpawnNPC(const GiftEvent& gift) {
    return SpawnNPC(gift.username, gift.oderId, -1);
}

SpawnResult NPCManager::SpawnNPC(const std::string& username, const std::string& oderId, int weaponId) {
    SpawnResult result;
    result.success = false;
    result.npc = nullptr;

    // Check if username already has an alive NPC
    if (!CanUsernameSpawn(username)) {
        result.message = username + " already has an active NPC!";
        LOG_WARNING("Spawn rejected: " + result.message);
        return result;
    }

    // Get spawn position
    Math::Vector3 spawnPos = GetRandomSpawnPosition();

    // Create the ped
    CPed* ped = CreatePed(spawnPos);
    if (!ped) {
        result.message = "Failed to create NPC for " + username;
        LOG_ERROR(result.message);
        return result;
    }

    // Create BattleNPC wrapper
    auto battleNPC = std::make_unique<BattleNPC>(username, oderId, ped);
    
    // Give weapon
    int finalWeaponId = weaponId;
    if (finalWeaponId < 0) {
        finalWeaponId = m_useRandomWeapons ? GetRandomWeapon() : m_defaultWeaponId;
    }
    battleNPC->GiveWeapon(finalWeaponId, 9999);

    // Store NPC
    BattleNPC* npcPtr = battleNPC.get();
    m_npcs.push_back(std::move(battleNPC));
    
    // Update username maps
    m_usernameToNPC[username] = npcPtr;
    m_usernameToAliveNPC[username] = npcPtr;

    // Notify callback
    if (m_spawnCallback) {
        m_spawnCallback(npcPtr);
    }

    result.success = true;
    result.message = "NPC spawned for " + username;
    result.npc = npcPtr;
    
    LOG_INFO("NPC spawned: " + username + " at (" + 
             std::to_string(spawnPos.x) + ", " + 
             std::to_string(spawnPos.y) + ", " + 
             std::to_string(spawnPos.z) + ")");
    
    return result;
}

BattleNPC* NPCManager::GetNPCByUsername(const std::string& username) {
    auto it = m_usernameToNPC.find(username);
    if (it != m_usernameToNPC.end()) {
        return it->second;
    }
    return nullptr;
}

BattleNPC* NPCManager::GetNPCByPed(CPed* ped) {
    for (auto& npc : m_npcs) {
        if (npc && npc->GetPed() == ped) {
            return npc.get();
        }
    }
    return nullptr;
}

std::vector<BattleNPC*> NPCManager::GetAllNPCs() {
    std::vector<BattleNPC*> result;
    for (auto& npc : m_npcs) {
        if (npc) {
            result.push_back(npc.get());
        }
    }
    return result;
}

std::vector<BattleNPC*> NPCManager::GetAliveNPCs() {
    std::vector<BattleNPC*> result;
    for (auto& npc : m_npcs) {
        if (npc && npc->IsAlive()) {
            result.push_back(npc.get());
        }
    }
    return result;
}

std::vector<BattleNPC*> NPCManager::GetDeadNPCs() {
    std::vector<BattleNPC*> result;
    for (auto& npc : m_npcs) {
        if (npc && npc->IsDead()) {
            result.push_back(npc.get());
        }
    }
    return result;
}

int NPCManager::GetAliveNPCCount() const {
    int count = 0;
    for (const auto& npc : m_npcs) {
        if (npc && npc->IsAlive()) {
            count++;
        }
    }
    return count;
}

int NPCManager::GetDeadNPCCount() const {
    int count = 0;
    for (const auto& npc : m_npcs) {
        if (npc && npc->IsDead()) {
            count++;
        }
    }
    return count;
}

void NPCManager::RemoveNPC(BattleNPC* npc) {
    if (!npc) return;

    // Remove from username maps
    m_usernameToNPC.erase(npc->GetUsername());
    m_usernameToAliveNPC.erase(npc->GetUsername());

    // Delete the ped from game world
    CPed* ped = npc->GetPed();
    if (ped) {
        CWorld::Remove(ped);
        delete ped;
    }

    // Remove from vector
    m_npcs.erase(
        std::remove_if(m_npcs.begin(), m_npcs.end(),
            [npc](const std::unique_ptr<BattleNPC>& p) { return p.get() == npc; }),
        m_npcs.end()
    );
}

void NPCManager::RemoveNPC(const std::string& username) {
    BattleNPC* npc = GetNPCByUsername(username);
    if (npc) {
        RemoveNPC(npc);
    }
}

void NPCManager::RemoveAllNPCs() {
    for (auto& npc : m_npcs) {
        if (npc) {
            CPed* ped = npc->GetPed();
            if (ped) {
                CWorld::Remove(ped);
                delete ped;
            }
        }
    }
    m_npcs.clear();
    m_usernameToNPC.clear();
    m_usernameToAliveNPC.clear();
    LOG_INFO("All NPCs removed");
}

void NPCManager::RemoveDeadNPCs() {
    // Collect dead NPCs
    std::vector<BattleNPC*> deadNPCs;
    for (auto& npc : m_npcs) {
        if (npc && npc->IsDead()) {
            deadNPCs.push_back(npc.get());
        }
    }
    
    // Remove them
    for (auto* npc : deadNPCs) {
        RemoveNPC(npc);
    }
}

void NPCManager::MakeAllNPCsEnemies() {
    auto aliveNPCs = GetAliveNPCs();
    for (auto* npc : aliveNPCs) {
        CPed* ped = npc->GetPed();
        if (ped) {
            // Set ped type to make them hostile
            // ePedType::PEDTYPE_GANG1 through GANG9 are typically hostile to each other
            // We'll set different gang types
        }
    }
    LOG_INFO("All NPCs set as enemies to each other");
}

void NPCManager::EnableAllCombatAI() {
    auto aliveNPCs = GetAliveNPCs();
    for (auto* npc : aliveNPCs) {
        npc->EnableCombatAI();
    }
    
    // Make each NPC target a random other NPC
    if (aliveNPCs.size() > 1) {
        for (auto* npc : aliveNPCs) {
            // Find a random target
            BattleNPC* target = nullptr;
            int attempts = 0;
            while (attempts < 10) {
                int idx = Math::RandomInt(0, static_cast<int>(aliveNPCs.size()) - 1);
                if (aliveNPCs[idx] != npc && aliveNPCs[idx]->IsAlive()) {
                    target = aliveNPCs[idx];
                    break;
                }
                attempts++;
            }
            
            if (target) {
                npc->SetTargetPed(target->GetPed());
            }
        }
    }
    
    LOG_INFO("Combat AI enabled for all NPCs");
}

void NPCManager::DisableAllCombatAI() {
    for (auto& npc : m_npcs) {
        if (npc) {
            npc->DisableCombatAI();
        }
    }
    LOG_INFO("Combat AI disabled for all NPCs");
}

CPed* NPCManager::CreatePed(const Math::Vector3& position) {
    // Use a random civilian model
    // Model IDs 9-288 are typical ped models in GTA SA
    static const int pedModels[] = { 7, 9, 10, 11, 12, 13, 14, 15, 16, 17, 19, 20, 21, 22, 23, 24, 25 };
    int modelId = pedModels[Math::RandomInt(0, sizeof(pedModels)/sizeof(pedModels[0]) - 1)];

    // Load model if needed
    CStreaming::RequestModel(modelId, STREAMING_GAME_REQUIRED);
    CStreaming::LoadAllRequestedModels(false);

    // Create the ped
    CVector pos(position.x, position.y, position.z);
    CPed* ped = new CCivilianPed(ePedType::PEDTYPE_CIVMALE, modelId);
    
    if (ped) {
        ped->SetPosn(pos);
        ped->SetOrientation(0, 0, Math::RandomFloat(0, Math::PI * 2));
        
        // Set initial health
        ped->m_fMaxHealth = 100.0f;
        ped->m_fHealth = 100.0f;
        
        // Add to world
        CWorld::Add(ped);
        
        // Set ped flags for combat
        ped->m_nPedFlags.bDontFight = false;
        ped->m_nPedFlags.bStayInSamePlace = false;
        
        return ped;
    }

    return nullptr;
}

Math::Vector3 NPCManager::GetRandomSpawnPosition() {
    Math::Vector3 pos = Math::RandomPointInCircle(m_spawnCenter, m_spawnRadius);
    
    // Find ground Z
    // In a real implementation, you'd use CWorld::FindGroundZForCoord
    // For now, we'll use the configured height
    const auto& spawnConfig = ConfigManager::GetInstance().GetSpawnConfig();
    pos.z = Math::RandomFloat(spawnConfig.minHeight, spawnConfig.maxHeight) + m_spawnCenter.z;
    
    return pos;
}

int NPCManager::GetRandomWeapon() {
    if (m_weaponList.empty()) {
        return m_defaultWeaponId;
    }
    int idx = Math::RandomInt(0, static_cast<int>(m_weaponList.size()) - 1);
    return m_weaponList[idx];
}

void NPCManager::ProcessDeaths() {
    for (auto& npc : m_npcs) {
        if (!npc) continue;
        
        // Check if NPC just died
        if (npc->GetPed() && npc->GetPed()->m_fHealth <= 0 && npc->IsAlive()) {
            // Mark as dead
            npc->SetState(NPCState::DEAD);
            
            // Try to find killer
            BattleNPC* killer = nullptr;
            CPed* ped = npc->GetPed();
            if (ped && ped->m_pLastEntityDamage) {
                killer = GetNPCByPed(static_cast<CPed*>(ped->m_pLastEntityDamage));
                if (killer) {
                    killer->IncrementKillCount();
                }
            }
            
            // Notify callback
            if (m_deathCallback) {
                m_deathCallback(npc.get(), killer);
            }
            
            LOG_INFO("NPC death detected: " + npc->GetUsername() + 
                     (killer ? " killed by " + killer->GetUsername() : ""));
        }
    }
}

void NPCManager::UpdateUsernameMap() {
    m_usernameToAliveNPC.clear();
    
    for (auto& npc : m_npcs) {
        if (npc && npc->IsAlive()) {
            m_usernameToAliveNPC[npc->GetUsername()] = npc.get();
        }
    }
}

} // namespace BattleGround
