#pragma once

#include "../utils/Math.h"
#include <string>
#include <cstdint>

// Forward declarations
class CPed;

namespace BattleGround {

enum class NPCState {
    ALIVE,
    DEAD,
    INVULNERABLE  // Temporary state after revive
};

class BattleNPC {
public:
    BattleNPC(const std::string& username, const std::string& oderId, CPed* ped);
    ~BattleNPC();

    // Getters
    const std::string& GetUsername() const { return m_username; }
    const std::string& GetUserId() const { return m_oderId; }
    CPed* GetPed() const { return m_ped; }
    NPCState GetState() const { return m_state; }
    bool IsAlive() const { return m_state == NPCState::ALIVE || m_state == NPCState::INVULNERABLE; }
    bool IsDead() const { return m_state == NPCState::DEAD; }
    bool IsInvulnerable() const { return m_state == NPCState::INVULNERABLE; }
    
    float GetHealth() const;
    float GetMaxHealth() const { return m_maxHealth; }
    Math::Vector3 GetPosition() const;
    
    int GetKillCount() const { return m_killCount; }
    int GetWeaponId() const { return m_weaponId; }

    // Setters
    void SetState(NPCState state) { m_state = state; }
    void SetWeaponId(int weaponId) { m_weaponId = weaponId; }
    
    // Actions
    void Update(float deltaTime);
    void GiveWeapon(int weaponId, int ammo);
    void SetHealth(float health);
    void Damage(float amount);
    void Kill();
    void Revive(float healthPercent);
    void SetInvulnerable(float duration);
    void IncrementKillCount() { m_killCount++; }
    
    // Combat
    void SetTargetPed();
    void ClearTarget();
    void EnableCombatAI();
    void DisableCombatAI();
    
    // Validation
    bool IsValid() const;

private:
    std::string m_username;
    std::string m_oderId;
    CPed* m_ped;
    NPCState m_state;
    
    float m_maxHealth;
    int m_weaponId;
    int m_killCount;
    
    // Invulnerability
    float m_invulnerabilityTimer;
    
    // Combat target
    CPed* m_targetPed;
};

} // namespace BattleGround
