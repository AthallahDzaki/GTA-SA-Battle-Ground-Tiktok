#include "CombatSystem.h"
#include "../npc/NPCManager.h"
#include "../utils/Logger.h"
#include "../utils/Math.h"

#include <CTheScripts.h>
#include <extensions/ScriptCommands.h>

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
    
    //UpdateCombat(deltaTime);
}

void CombatSystem::Shutdown() {
    DisableCombat();
    m_initialized = false;
    LOG_INFO("Combat System shutdown");
}

int CombatSystem::CreateDecisionMaker() {
    int decisionID = -10;
    decisionID = plugin::Command<plugin::Commands::COPY_SHARED_CHAR_DECISION_MAKER>(65542);
    if (decisionID == -10) return -10;
    plugin::Command<plugin::Commands::ADD_CHAR_DECISION_MAKER_EVENT_RESPONSE>(decisionID, 31, 1002, 0.0, 100.0, 0.0, 100.0, false, true);
    return decisionID;
}

void CombatSystem::EnableCombat() {
    m_combatEnabled = true;
    auto npcs = NPCManager::GetInstance().GetAllNPCs();
    for (auto* npc : npcs) {
        if (npc) {
            npc->SetTargetPed();
        }
    }
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

void CombatSystem::UpdateCombat(float deltaTime) {
    // Periodically retarget (every 2 seconds)
}

} // namespace BattleGround
