#include "BattleNPC.h"
#include "../utils/Logger.h"

// GTA SA SDK includes
#include <CPed.h>
#include <CWorld.h>
#include <CWeapon.h>
#include <CTaskComplexKillPedOnFoot.h>
#include <CTaskManager.h>

namespace BattleGround {

BattleNPC::BattleNPC(const std::string& username, const std::string& oderId, CPed* ped)
    : m_username(username)
    , m_oderId(oderId)
    , m_ped(ped)
    , m_state(NPCState::ALIVE)
    , m_maxHealth(100.0f)
    , m_weaponId(0)
    , m_killCount(0)
    , m_invulnerabilityTimer(0)
    , m_targetPed(nullptr)
{
    if (m_ped) {
        m_maxHealth = m_ped->m_fMaxHealth;
    }
    LOG_DEBUG("BattleNPC created for user: " + username);
}

BattleNPC::~BattleNPC() {
    // Don't delete the ped here - NPCManager handles cleanup
    LOG_DEBUG("BattleNPC destroyed for user: " + m_username);
}

float BattleNPC::GetHealth() const {
    if (m_ped && IsValid()) {
        return m_ped->m_fHealth;
    }
    return 0.0f;
}

Math::Vector3 BattleNPC::GetPosition() const {
    if (m_ped && IsValid()) {
        CVector pos = m_ped->GetPosition();
        return Math::Vector3(pos.x, pos.y, pos.z);
    }
    return Math::Vector3(0, 0, 0);
}

void BattleNPC::Update(float deltaTime) {
    if (!IsValid()) {
        if (m_state != NPCState::DEAD) {
            m_state = NPCState::DEAD;
        }
        return;
    }

    // Update invulnerability timer
    if (m_state == NPCState::INVULNERABLE) {
        m_invulnerabilityTimer -= deltaTime;
        if (m_invulnerabilityTimer <= 0) {
            m_state = NPCState::ALIVE;
            LOG_DEBUG("Invulnerability ended for: " + m_username);
        }
    }

    // Check if ped died
    if (m_ped && m_ped->m_fHealth <= 0 && m_state != NPCState::DEAD) {
        m_state = NPCState::DEAD;
        LOG_INFO("NPC died: " + m_username);
    }
}

void BattleNPC::GiveWeapon(int weaponId, int ammo) {
    if (m_ped && IsValid()) {
        m_ped->GiveWeapon((eWeaponType)weaponId, ammo, true);
        m_ped->SetCurrentWeapon((eWeaponType)weaponId);
        m_weaponId = weaponId;
        LOG_DEBUG("Gave weapon " + std::to_string(weaponId) + " to " + m_username);
    }
}

void BattleNPC::SetHealth(float health) {
    if (m_ped && IsValid()) {
        m_ped->m_fHealth = health;
    }
}

void BattleNPC::Damage(float amount) {
    if (m_state == NPCState::INVULNERABLE) {
        return; // Can't damage invulnerable NPCs
    }
    
    if (m_ped && IsValid()) {
        float newHealth = m_ped->m_fHealth - amount;
        if (newHealth <= 0) {
            Kill();
        } else {
            m_ped->m_fHealth = newHealth;
        }
    }
}

void BattleNPC::Kill() {
    if (m_ped && IsValid()) {
        m_ped->m_fHealth = 0;
        m_state = NPCState::DEAD;
        LOG_INFO("NPC killed: " + m_username);
    }
}

void BattleNPC::Revive(float healthPercent) {
    if (m_ped) {
        // Resurrect the ped if possible
        float health = m_maxHealth * (healthPercent / 100.0f);
        m_ped->m_fHealth = health;
        
        // Reset ped state
        m_ped->m_ePedState = ePedState::PEDSTATE_IDLE;
        
        m_state = NPCState::ALIVE;
        LOG_INFO("NPC revived: " + m_username + " with " + std::to_string(healthPercent) + "% health");
    }
}

void BattleNPC::SetInvulnerable(float duration) {
    m_state = NPCState::INVULNERABLE;
    m_invulnerabilityTimer = duration;
    LOG_DEBUG("NPC " + m_username + " is now invulnerable for " + std::to_string(duration) + " seconds");
}

void BattleNPC::SetTargetPed(CPed* target) {
    m_targetPed = target;
    if (m_ped && target && IsValid()) {
        // Create kill task
        auto* task = new CTaskComplexKillPedOnFoot(target, -1, 0, 0, 0, 2);
        if (task && m_ped->m_pIntelligence) {
            m_ped->m_pIntelligence->m_TaskMgr.SetTask(task, 0, false);
        }
    }
}

void BattleNPC::ClearTarget() {
    m_targetPed = nullptr;
    if (m_ped && m_ped->m_pIntelligence) {
        m_ped->m_pIntelligence->m_TaskMgr.Flush();
    }
}

void BattleNPC::EnableCombatAI() {
    if (m_ped && IsValid()) {
        // Make ped aggressive
        m_ped->m_nPedFlags.bDontFight = false;
        m_ped->m_nPedFlags.bStayInSamePlace = false;
        
        // Set ped to combat state
        m_ped->m_ePedState = ePedState::PEDSTATE_ATTACK;
        
        LOG_DEBUG("Combat AI enabled for: " + m_username);
    }
}

void BattleNPC::DisableCombatAI() {
    if (m_ped && IsValid()) {
        ClearTarget();
        m_ped->m_ePedState = ePedState::PEDSTATE_IDLE;
        LOG_DEBUG("Combat AI disabled for: " + m_username);
    }
}

bool BattleNPC::IsValid() const {
    if (!m_ped) return false;
    
    // Check if ped is still in the world
    // This is a simple validity check
    return m_ped->m_fHealth > 0 || m_state == NPCState::DEAD;
}

} // namespace BattleGround
