#pragma once

#include "../npc/BattleNPC.h"
#include "../network/WebSocketClient.h"
#include <vector>
#include <string>
#include <functional>

namespace BattleGround {

// Revive result for UI feedback
struct ReviveResult {
    bool success;
    std::string message;
    BattleNPC* npc;  // nullptr if failed
};

class ReviveSystem {
public:
    static ReviveSystem& GetInstance() {
        static ReviveSystem instance;
        return instance;
    }

    void Initialize();
    void Update(float deltaTime);
    void Shutdown();

    // Attempt to revive with a gift
    ReviveResult TryRevive(const GiftEvent& gift);
    ReviveResult TryRevive(const std::string& username, const std::string& giftId);

    // Check if gift can be used for revive
    bool IsReviveGift(const std::string& giftId) const;
    bool CanRevive(const std::string& username) const;

    // Callbacks
    using ReviveCallback = std::function<void(BattleNPC* npc)>;
    void SetReviveCallback(ReviveCallback callback) { m_reviveCallback = callback; }

private:
    ReviveSystem() = default;
    ~ReviveSystem() = default;
    ReviveSystem(const ReviveSystem&) = delete;
    ReviveSystem& operator=(const ReviveSystem&) = delete;

    // Config
    std::vector<std::string> m_reviveGiftIds;
    int m_reviveHealthPercent = 50;
    int m_invulnerabilitySeconds = 3;
    bool m_requireOwnerMatch = true;

    ReviveCallback m_reviveCallback;
    bool m_initialized = false;
};

} // namespace BattleGround
