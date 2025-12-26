#include "ReviveSystem.h"
#include "../npc/NPCManager.h"
#include "../match/MatchController.h"
#include "../config/ConfigManager.h"
#include "../utils/Logger.h"

#include <algorithm>

namespace BattleGround {

void ReviveSystem::Initialize() {
    if (m_initialized) return;

    const auto& config = ConfigManager::GetInstance().GetReviveConfig();
    m_reviveGiftIds = config.reviveGiftIds;
    m_reviveHealthPercent = config.reviveHealthPercent;
    m_invulnerabilitySeconds = config.invulnerabilitySeconds;
    m_requireOwnerMatch = config.requireOwnerMatch;

    m_initialized = true;
    LOG_INFO("Revive System initialized with " + std::to_string(m_reviveGiftIds.size()) + " revive gifts");
}

void ReviveSystem::Update(float deltaTime) {
    // No continuous updates needed
}

void ReviveSystem::Shutdown() {
    m_initialized = false;
    LOG_INFO("Revive System shutdown");
}

bool ReviveSystem::IsReviveGift(const std::string& giftId) const {
    return std::find(m_reviveGiftIds.begin(), m_reviveGiftIds.end(), giftId) != m_reviveGiftIds.end();
}

bool ReviveSystem::CanRevive(const std::string& username) const {
    // Check if username has a dead NPC
    BattleNPC* npc = NPCManager::GetInstance().GetNPCByUsername(username);
    if (!npc || !npc->IsDead()) {
        return false;
    }

    // Check match state
    auto state = MatchController::GetInstance().GetState();
    if (state != MatchState::IN_PROGRESS && state != MatchState::WINNER_DELAY) {
        return false;
    }

    return true;
}

ReviveResult ReviveSystem::TryRevive(const GiftEvent& gift) {
    return TryRevive(gift.username, gift.giftId);
}

ReviveResult ReviveSystem::TryRevive(const std::string& username, const std::string& giftId) {
    ReviveResult result;
    result.success = false;
    result.npc = nullptr;

    // Check if gift is a revive gift
    if (!IsReviveGift(giftId)) {
        result.message = "Gift '" + giftId + "' is not a revive gift";
        return result;
    }

    // Check match state
    auto state = MatchController::GetInstance().GetState();
    if (state != MatchState::IN_PROGRESS && state != MatchState::WINNER_DELAY) {
        result.message = "Match is not active - cannot revive";
        return result;
    }

    // Find the NPC to revive
    BattleNPC* npc = nullptr;
    
    if (m_requireOwnerMatch) {
        // Revive the sender's own NPC
        npc = NPCManager::GetInstance().GetNPCByUsername(username);
        if (!npc) {
            result.message = username + " does not have an NPC to revive";
            return result;
        }
        if (!npc->IsDead()) {
            result.message = username + "'s NPC is not dead";
            return result;
        }
    } else {
        // Find any dead NPC (less common use case)
        auto deadNPCs = NPCManager::GetInstance().GetDeadNPCs();
        if (deadNPCs.empty()) {
            result.message = "No dead NPCs to revive";
            return result;
        }
        npc = deadNPCs[0]; // Revive first dead NPC
    }

    // Perform the revive
    npc->Revive(static_cast<float>(m_reviveHealthPercent));
    npc->SetInvulnerable(static_cast<float>(m_invulnerabilitySeconds));
    npc->EnableCombatAI();
    npc->SetTargetPed();  // Set HATE relationship to make NPC attack others

    // Notify match controller
    MatchController::GetInstance().OnNPCRevived();

    // Callback
    if (m_reviveCallback) {
        m_reviveCallback(npc);
    }

    result.success = true;
    result.message = npc->GetUsername() + " has been revived!";
    result.npc = npc;

    LOG_INFO("NPC revived: " + npc->GetUsername() + " by gift from " + username);
    return result;
}

} // namespace BattleGround
