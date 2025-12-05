#include "MatchController.h"
#include "../npc/NPCManager.h"
#include "../config/ConfigManager.h"
#include "../utils/Logger.h"

namespace BattleGround {

void MatchController::Initialize() {
    if (m_initialized) return;

    const auto& config = ConfigManager::GetInstance().GetMatchConfig();
    m_minNPCs = config.minNPCsToStart;
    m_countdownSeconds = config.countdownSeconds;
    m_winnerDelaySeconds = config.winnerDelaySeconds;
    m_autoStart = config.autoStartMatch;

    m_state = MatchState::WAITING;
    m_winner = nullptr;
    m_countdown = 0;
    m_winnerDelay = 0;

    m_initialized = true;
    LOG_INFO("Match Controller initialized");
}

void MatchController::Update(float deltaTime) {
    switch (m_state) {
        case MatchState::WAITING:
            CheckMatchConditions();
            break;

        case MatchState::COUNTDOWN:
            m_countdown -= deltaTime;
            if (m_countdown <= 0) {
                StartMatch();
            }
            break;

        case MatchState::IN_PROGRESS:
            CheckWinCondition();
            break;

        case MatchState::WINNER_DELAY:
            m_winnerDelay -= deltaTime;
            if (m_winnerDelay <= 0) {
                // Declare winner
                if (m_winner && m_winnerCallback) {
                    m_winnerCallback(m_winner);
                }
                SetState(MatchState::FINISHED);
            }
            // Still check if more NPCs join
            CheckWinCondition();
            break;

        case MatchState::FINISHED:
            // Wait for reset
            break;
    }
}

void MatchController::Shutdown() {
    m_initialized = false;
    LOG_INFO("Match Controller shutdown");
}

void MatchController::Reset() {
    SetState(MatchState::WAITING);
    m_winner = nullptr;
    m_countdown = 0;
    m_winnerDelay = 0;
    LOG_INFO("Match reset");
}

std::string MatchController::GetStateString() const {
    switch (m_state) {
        case MatchState::WAITING:       return "WAITING";
        case MatchState::COUNTDOWN:     return "COUNTDOWN";
        case MatchState::IN_PROGRESS:   return "IN PROGRESS";
        case MatchState::WINNER_DELAY:  return "DECIDING WINNER";
        case MatchState::FINISHED:      return "FINISHED";
        default:                        return "UNKNOWN";
    }
}

bool MatchController::IsMatchActive() const {
    return m_state == MatchState::IN_PROGRESS || 
           m_state == MatchState::WINNER_DELAY;
}

void MatchController::StartMatch() {
    SetState(MatchState::IN_PROGRESS);
    
    // Enable combat for all NPCs
    NPCManager::GetInstance().EnableAllCombatAI();
    NPCManager::GetInstance().MakeAllNPCsEnemies();
    
    LOG_INFO("Match started!");
}

void MatchController::EndMatch() {
    // Disable combat
    NPCManager::GetInstance().DisableAllCombatAI();
    
    SetState(MatchState::FINISHED);
    LOG_INFO("Match ended");
}

void MatchController::ForceStart() {
    if (m_state == MatchState::WAITING || m_state == MatchState::COUNTDOWN) {
        m_countdown = 0;
        StartMatch();
    }
}

void MatchController::SetWinner(BattleNPC* winner) {
    m_winner = winner;
    if (winner) {
        LOG_INFO("Winner set: " + winner->GetUsername());
    }
}

void MatchController::OnNPCCountChanged() {
    if (m_state == MatchState::WAITING && m_autoStart) {
        CheckMatchConditions();
    }
}

void MatchController::OnNPCRevived() {
    // If we're in WINNER_DELAY and an NPC was revived, go back to IN_PROGRESS
    if (m_state == MatchState::WINNER_DELAY) {
        int aliveCount = NPCManager::GetInstance().GetAliveNPCCount();
        if (aliveCount > 1) {
            SetState(MatchState::IN_PROGRESS);
            m_winner = nullptr;
            LOG_INFO("NPC revived during winner delay - match continues!");
        }
    }
}

void MatchController::SetState(MatchState state) {
    if (m_state == state) return;
    
    MatchState oldState = m_state;
    m_state = state;
    
    LOG_INFO("Match state changed: " + GetStateString());
    
    if (m_stateChangeCallback) {
        m_stateChangeCallback(oldState, state);
    }
}

void MatchController::CheckMatchConditions() {
    if (m_state != MatchState::WAITING) return;
    
    int aliveCount = NPCManager::GetInstance().GetAliveNPCCount();
    
    if (aliveCount >= m_minNPCs && m_autoStart) {
        // Start countdown
        m_countdown = static_cast<float>(m_countdownSeconds);
        SetState(MatchState::COUNTDOWN);
        LOG_INFO("Countdown started: " + std::to_string(m_countdownSeconds) + " seconds");
    }
}

void MatchController::CheckWinCondition() {
    int aliveCount = NPCManager::GetInstance().GetAliveNPCCount();
    
    if (m_state == MatchState::IN_PROGRESS) {
        if (aliveCount <= 1) {
            // One or zero NPCs left
            auto aliveNPCs = NPCManager::GetInstance().GetAliveNPCs();
            if (!aliveNPCs.empty()) {
                m_winner = aliveNPCs[0];
            }
            
            // Enter winner delay period (allows revives)
            m_winnerDelay = static_cast<float>(m_winnerDelaySeconds);
            SetState(MatchState::WINNER_DELAY);
            LOG_INFO("Winner delay started: " + std::to_string(m_winnerDelaySeconds) + " seconds");
        }
    } else if (m_state == MatchState::WINNER_DELAY) {
        if (aliveCount > 1) {
            // Someone was revived, continue match
            SetState(MatchState::IN_PROGRESS);
            m_winner = nullptr;
            LOG_INFO("Match continues - multiple NPCs alive");
        }
    }
}

} // namespace BattleGround
