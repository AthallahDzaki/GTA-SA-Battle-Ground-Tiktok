#pragma once

#include "../npc/BattleNPC.h"
#include <string>
#include <functional>

namespace BattleGround {

enum class MatchState {
    WAITING,        // Waiting for minimum NPCs
    COUNTDOWN,      // Countdown before match starts
    IN_PROGRESS,    // Match is active
    WINNER_DELAY,   // Waiting period before declaring winner
    FINISHED        // Match ended
};

class MatchController {
public:
    static MatchController& GetInstance() {
        static MatchController instance;
        return instance;
    }

    void Initialize();
    void Update(float deltaTime);
    void Shutdown();
    void Reset();

    // State
    MatchState GetState() const { return m_state; }
    std::string GetStateString() const;
    bool IsMatchActive() const;

    // Match control
    void StartMatch();
    void EndMatch();
    void ForceStart();  // Start regardless of NPC count

    // Timers
    float GetCountdown() const { return m_countdown; }
    float GetWinnerDelay() const { return m_winnerDelay; }
    int GetMinNPCsRequired() const { return m_minNPCs; }

    // Winner
    BattleNPC* GetWinner() const { return m_winner; }
    void SetWinner(BattleNPC* winner);

    // Callbacks
    using StateChangeCallback = std::function<void(MatchState oldState, MatchState newState)>;
    using WinnerCallback = std::function<void(BattleNPC* winner)>;
    
    void SetStateChangeCallback(StateChangeCallback callback) { m_stateChangeCallback = callback; }
    void SetWinnerCallback(WinnerCallback callback) { m_winnerCallback = callback; }

    // Called when NPC count changes
    void OnNPCCountChanged();
    void OnNPCRevived();

private:
    MatchController() = default;
    ~MatchController() = default;
    MatchController(const MatchController&) = delete;
    MatchController& operator=(const MatchController&) = delete;

    void SetState(MatchState state);
    void CheckMatchConditions();
    void CheckWinCondition();

    MatchState m_state = MatchState::WAITING;
    
    // Config
    int m_minNPCs = 2;
    int m_countdownSeconds = 5;
    int m_winnerDelaySeconds = 10;
    bool m_autoStart = true;

    // Timers
    float m_countdown = 0;
    float m_winnerDelay = 0;
    float m_nextMatchDelay = 0;

    // Winner
    BattleNPC* m_winner = nullptr;

    // Callbacks
    StateChangeCallback m_stateChangeCallback;
    WinnerCallback m_winnerCallback;

    bool m_initialized = false;
};

} // namespace BattleGround
