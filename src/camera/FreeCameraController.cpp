#include "FreeCameraController.h"
#include "../utils/Logger.h"

// GTA SA SDK includes
#include <CCamera.h>
#include <CWorld.h>
#include <CPlayerPed.h>
#include <CPad.h>

namespace BattleGround {

FreeCameraController::FreeCameraController()
    : m_enabled(false)
    , m_initialized(false)
    , m_position(0, 0, 50)
    , m_pitch(0)
    , m_yaw(0)
    , m_moveForward(false)
    , m_moveBackward(false)
    , m_moveLeft(false)
    , m_moveRight(false)
    , m_moveUp(false)
    , m_moveDown(false)
    , m_moveFast(false)
    , m_moveSlow(false)
    , m_mouseDeltaX(0)
    , m_mouseDeltaY(0)
    , m_hasTarget(false)
    , m_targetPosition(0, 0, 0)
    , m_targetDistance(15.0f)
    , m_moveSpeed(1.0f)
    , m_fastMultiplier(3.0f)
    , m_slowMultiplier(0.3f)
    , m_mouseSensitivity(0.002f)
    , m_autoFollowEnabled(false)
    , m_autoFollowDistance(15.0f)
{
}

void FreeCameraController::Initialize() {
    if (m_initialized) return;

    const auto& config = ConfigManager::GetInstance().GetCameraConfig();
    m_moveSpeed = config.moveSpeed;
    m_fastMultiplier = config.fastSpeedMultiplier;
    m_slowMultiplier = config.slowSpeedMultiplier;
    m_mouseSensitivity = config.mouseSensitivity;
    m_autoFollowEnabled = config.autoFollowEnabled;
    m_autoFollowDistance = config.autoFollowDistance;

    // Set initial position to spawn area center
    const auto& spawnConfig = ConfigManager::GetInstance().GetSpawnConfig();
    m_position = Math::Vector3(spawnConfig.centerX, spawnConfig.centerY, spawnConfig.centerZ + 50.0f);

    m_initialized = true;
    LOG_INFO("Free camera controller initialized");
}

void FreeCameraController::Update(float deltaTime) {
    if (!m_enabled || !m_initialized) return;

    // Disable player control when camera is active
    DisablePlayerControl();

    // Process mouse rotation
    m_pitch += m_mouseDeltaY * m_mouseSensitivity;
    m_yaw += m_mouseDeltaX * m_mouseSensitivity;

    // Clamp pitch to prevent flipping
    float cx = -3.14159265358979323846f * 0.49f;
    float cy = 3.14159265358979323846f * 0.49f;
    m_pitch = Math::Clamp(m_pitch, cx, cy);

    // Reset mouse delta
    m_mouseDeltaX = 0;
    m_mouseDeltaY = 0;

    // Update camera based on mode
    if (m_hasTarget && m_autoFollowEnabled) {
        UpdateAutoFollow(deltaTime);
    } else {
        UpdateFreeCamera(deltaTime);
    }

    // Apply camera position to game
    ApplyCameraToGame();
}

void FreeCameraController::Shutdown() {
    m_enabled = false;
    m_initialized = false;
    LOG_INFO("Free camera controller shutdown");
}

void FreeCameraController::SetRotation(float pitch, float yaw) {
    m_pitch = pitch;
    m_yaw = yaw;
}

void FreeCameraController::SetTarget(const Math::Vector3& target) {
    m_hasTarget = true;
    m_targetPosition = target;
}

void FreeCameraController::ClearTarget() {
    m_hasTarget = false;
}

void FreeCameraController::HandleMouseMovement(float deltaX, float deltaY) {
    m_mouseDeltaX += deltaX;
    m_mouseDeltaY += deltaY;
}

void FreeCameraController::HandleKeyboardInput(bool forward, bool backward, bool left, bool right, 
                                                bool up, bool down, bool fast, bool slow) {
    m_moveForward = forward;
    m_moveBackward = backward;
    m_moveLeft = left;
    m_moveRight = right;
    m_moveUp = up;
    m_moveDown = down;
    m_moveFast = fast;
    m_moveSlow = slow;
}

void FreeCameraController::UpdateFreeCamera(float deltaTime) {
    // Calculate forward and right vectors based on yaw
    Math::Vector3 forward(
        std::cos(m_yaw) * std::cos(m_pitch),
        std::sin(m_yaw) * std::cos(m_pitch),
        std::sin(m_pitch)
    );
    
    Math::Vector3 right(
        std::cos(m_yaw - 3.14159265358979323846f * 0.5f),
        std::sin(m_yaw - 3.14159265358979323846f * 0.5f),
        0
    );

    Math::Vector3 up(0, 0, 1);

    // Calculate movement direction
    Math::Vector3 movement(0, 0, 0);
    
    if (m_moveForward) movement = movement + forward;
    if (m_moveBackward) movement = movement - forward;
    if (m_moveRight) movement = movement + right;
    if (m_moveLeft) movement = movement - right;
    if (m_moveUp) movement = movement + up;
    if (m_moveDown) movement = movement - up;

    // Apply speed modifiers
    float speed = m_moveSpeed;
    if (m_moveFast) speed *= m_fastMultiplier;
    if (m_moveSlow) speed *= m_slowMultiplier;

    // Move camera
    if (movement.LengthSquared() > 0.001f) {
        movement = movement.Normalized();
        m_position = m_position + movement * (speed * deltaTime * 50.0f);
    }
}

void FreeCameraController::UpdateAutoFollow(float deltaTime) {
    // Calculate desired position behind target
    Math::Vector3 offset(
        -std::cos(m_yaw) * m_autoFollowDistance,
        -std::sin(m_yaw) * m_autoFollowDistance,
        m_autoFollowDistance * 0.5f
    );

    Math::Vector3 desiredPosition = m_targetPosition + offset;

    // Smooth follow
    m_position = Math::Lerp(m_position, desiredPosition, Math::Clamp01(deltaTime * 5.0f));
}

void FreeCameraController::ApplyCameraToGame() {
    // Get the game camera
    CCamera* camera = &TheCamera;
    if (!camera) return;

    // Create source and target points
    CVector source(m_position.x, m_position.y, m_position.z);
    
    // Calculate look direction
    Math::Vector3 lookDir(
        std::cos(m_yaw) * std::cos(m_pitch),
        std::sin(m_yaw) * std::cos(m_pitch),
        std::sin(m_pitch)
    );
    
    CVector target(
        m_position.x + lookDir.x * 100.0f,
        m_position.y + lookDir.y * 100.0f,
        m_position.z + lookDir.z * 100.0f
    );

    // Set camera to fixed mode
    camera->SetCamPositionForFixedMode(&source, &target);
    camera->TakeControl(nullptr, eCamMode::MODE_FIXED, 1, 1);
}

void FreeCameraController::DisablePlayerControl() {
    // Disable player input
    CPad* pad = CPad::GetPad(0);
    if (pad) {
        pad->DisablePlayerControls = 1;
    }
}

} // namespace BattleGround
