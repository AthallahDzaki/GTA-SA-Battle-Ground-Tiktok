#pragma once

#include "../utils/Math.h"
#include "../config/ConfigManager.h"

namespace BattleGround {

class FreeCameraController {
public:
    static FreeCameraController& GetInstance() {
        static FreeCameraController instance;
        return instance;
    }

    void Initialize();
    void Update(float deltaTime);
    void Shutdown();

    void SetEnabled(bool enabled) { m_enabled = enabled; }
    bool IsEnabled() const { return m_enabled; }

    void SetPosition(const Math::Vector3& position) { m_position = position; }
    Math::Vector3 GetPosition() const { return m_position; }

    void SetRotation(float pitch, float yaw);
    float GetPitch() const { return m_pitch; }
    float GetYaw() const { return m_yaw; }

    void SetTarget(const Math::Vector3& target);
    void ClearTarget();
    bool HasTarget() const { return m_hasTarget; }

    // Input handling
    void HandleMouseMovement(float deltaX, float deltaY);
    void HandleKeyboardInput(bool forward, bool backward, bool left, bool right, bool up, bool down, bool fast, bool slow);

private:
    FreeCameraController();
    ~FreeCameraController() = default;
    FreeCameraController(const FreeCameraController&) = delete;
    FreeCameraController& operator=(const FreeCameraController&) = delete;

    void UpdateFreeCamera(float deltaTime);
    void UpdateAutoFollow(float deltaTime);
    void ApplyCameraToGame();
    void DisablePlayerControl();

    bool m_enabled;
    bool m_initialized;
    
    Math::Vector3 m_position;
    float m_pitch;  // Up/down rotation
    float m_yaw;    // Left/right rotation

    // Movement input state
    bool m_moveForward;
    bool m_moveBackward;
    bool m_moveLeft;
    bool m_moveRight;
    bool m_moveUp;
    bool m_moveDown;
    bool m_moveFast;
    bool m_moveSlow;

    // Mouse movement accumulator
    float m_mouseDeltaX;
    float m_mouseDeltaY;

    // Auto-follow
    bool m_hasTarget;
    Math::Vector3 m_targetPosition;
    float m_targetDistance;

    // Config cache
    float m_moveSpeed;
    float m_fastMultiplier;
    float m_slowMultiplier;
    float m_mouseSensitivity;
    bool m_autoFollowEnabled;
    float m_autoFollowDistance;
};

} // namespace BattleGround
