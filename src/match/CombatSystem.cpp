#include "CombatSystem.h"
#include "../npc/NPCManager.h"
#include "../utils/Logger.h"
#include "../utils/Math.h"

namespace BattleGround {

void CombatSystem::Initialize() {
    if (m_initialized) return;
    
    m_combatEnabled = false;
    m_totalKills = 0;
    m_retargetTimer = 0;
    
    m_initialized = true;
    LOG_INFO("Combat System initialized");
}

void CombatSystem::Update(float deltaTime) {
    if (!m_combatEnabled) return;
    
    UpdateCombat(deltaTime);
}

void CombatSystem::Shutdown() {
    DisableCombat();
    m_initialized = false;
    LOG_INFO("Combat System shutdown");
}

void CombatSystem::EnableCombat() {
    m_combatEnabled = true;
    AssignTargets();
    LOG_INFO("Combat enabled");
}

void CombatSystem::DisableCombat() {
    m_combatEnabled = false;
    
    // Clear all targets
    auto npcs = NPCManager::GetInstance().GetAllNPCs();
    for (auto* npc : npcs) {
        if (npc) {
            npc->ClearTarget();
        }
    }
    
    LOG_INFO("Combat disabled");
}

void CombatSystem::AssignTargets() {
    auto aliveNPCs = NPCManager::GetInstance().GetAliveNPCs();
    
    for (auto* npc : aliveNPCs) {
        if (!npc || !npc->IsAlive()) continue;
        
        BattleNPC* target = FindNearestEnemy(npc);
        if (target) {
            npc->SetTargetPed(target->GetPed());
        }
    }
    
    LOG_DEBUG("Targets assigned to all alive NPCs");
}

void CombatSystem::RetargetDeadTargets() {
    auto aliveNPCs = NPCManager::GetInstance().GetAliveNPCs();
    
    for (auto* npc : aliveNPCs) {
        if (!npc || !npc->IsAlive()) continue;
        
        // Check if current target is dead or invalid
        // For now, just reassign everyone
        BattleNPC* target = FindNearestEnemy(npc);
        if (target) {
            npc->SetTargetPed(target->GetPed());
        }
    }
}

void CombatSystem::UpdateCombat(float deltaTime) {
    // Periodically retarget (every 2 seconds)
    m_retargetTimer += deltaTime;
    if (m_retargetTimer >= 2.0f) {
        m_retargetTimer = 0;
        RetargetDeadTargets();
    }
}

BattleNPC* CombatSystem::FindNearestEnemy(BattleNPC* npc) {
    if (!npc) return nullptr;
    
    auto aliveNPCs = NPCManager::GetInstance().GetAliveNPCs();
    BattleNPC* nearest = nullptr;
    float nearestDist = FLT_MAX;
    
    Math::Vector3 myPos = npc->GetPosition();
    
    for (auto* other : aliveNPCs) {
        if (!other || other == npc || !other->IsAlive()) continue;
        
        Math::Vector3 otherPos = other->GetPosition();
        float dist = Math::DistanceSquared(myPos, otherPos);
        
        if (dist < nearestDist) {
            nearestDist = dist;
            nearest = other;
        }
    }
    
    return nearest;
}

} // namespace BattleGround
