#pragma once

#include "../npc/BattleNPC.h"
#include <vector>

namespace BattleGround {

class CombatSystem {
public:
    static CombatSystem& GetInstance() {
        static CombatSystem instance;
        return instance;
    }

    void Initialize();
    void Update(float deltaTime);
    void Shutdown();

    // Combat control
    void EnableCombat();
    void DisableCombat();
    bool IsCombatEnabled() const { return m_combatEnabled; }

    // Make NPCs fight each other
    void AssignTargets();
    void RetargetDeadTargets();

    // Stats
    int GetTotalKills() const { return m_totalKills; }

private:
    CombatSystem() = default;
    ~CombatSystem() = default;
    CombatSystem(const CombatSystem&) = delete;
    CombatSystem& operator=(const CombatSystem&) = delete;

    void UpdateCombat(float deltaTime);
    BattleNPC* FindNearestEnemy(BattleNPC* npc);

    bool m_combatEnabled = false;
    float m_retargetTimer = 0;
    int m_totalKills = 0;
    bool m_initialized = false;
};

} // namespace BattleGround
