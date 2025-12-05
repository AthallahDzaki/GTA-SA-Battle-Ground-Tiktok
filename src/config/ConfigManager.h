#pragma once

#include <string>
#include <map>
#include <vector>
#include <sstream>

namespace BattleGround {

struct MatchConfig {
    int minNPCsToStart = 2;
    int countdownSeconds = 5;
    int winnerDelaySeconds = 10;
    bool autoStartMatch = true;
};

struct SpawnConfig {
    float centerX = 2488.0f;
    float centerY = -1666.0f;
    float centerZ = 13.5f;
    float spawnRadius = 50.0f;
    float minHeight = 10.0f;
    float maxHeight = 20.0f;
    bool oneNPCPerUsername = true;
};

struct WebSocketConfig {
    std::string serverURL = "ws://localhost:8080";
    bool autoReconnect = true;
    int reconnectIntervalSeconds = 5;
    int connectionTimeout = 10;
};

struct ReviveConfig {
    std::vector<std::string> reviveGiftIds = {"diamond", "rose_gold", "galaxy"};
    int reviveHealthPercent = 50;
    int invulnerabilitySeconds = 3;
    bool requireOwnerMatch = true;
};

struct WeaponsConfig {
    int defaultWeaponId = 22;
    bool useRandomWeapons = true;
    std::vector<int> weaponList = {22, 24, 25, 28, 29, 30, 31};
};

struct CameraConfig {
    float moveSpeed = 1.0f;
    float fastSpeedMultiplier = 3.0f;
    float slowSpeedMultiplier = 0.3f;
    float mouseSensitivity = 0.002f;
    bool autoFollowEnabled = false;
    float autoFollowDistance = 15.0f;
};

struct UIConfig {
    bool showNPCNames = true;
    bool showHealthBars = true;
    bool showKillCount = true;
    bool showMatchStatus = true;
    float notificationDuration = 5.0f;
    bool showGiftNotifications = true;
};

class ConfigManager {
public:
    static ConfigManager& GetInstance() {
        static ConfigManager instance;
        return instance;
    }

    bool Load(const std::string& filename);
    bool Save(const std::string& filename);

    const MatchConfig& GetMatchConfig() const { return m_match; }
    const SpawnConfig& GetSpawnConfig() const { return m_spawn; }
    const WebSocketConfig& GetWebSocketConfig() const { return m_websocket; }
    const ReviveConfig& GetReviveConfig() const { return m_revive; }
    const WeaponsConfig& GetWeaponsConfig() const { return m_weapons; }
    const CameraConfig& GetCameraConfig() const { return m_camera; }
    const UIConfig& GetUIConfig() const { return m_ui; }

private:
    ConfigManager() = default;
    ~ConfigManager() = default;
    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;

    std::string GetValue(const std::string& section, const std::string& key, const std::string& defaultValue);
    int GetInt(const std::string& section, const std::string& key, int defaultValue);
    float GetFloat(const std::string& section, const std::string& key, float defaultValue);
    bool GetBool(const std::string& section, const std::string& key, bool defaultValue);
    std::vector<std::string> GetStringList(const std::string& section, const std::string& key, const std::vector<std::string>& defaultValue);
    std::vector<int> GetIntList(const std::string& section, const std::string& key, const std::vector<int>& defaultValue);

    void ParseIniFile(const std::string& content);
    static std::string Trim(const std::string& str);

    std::map<std::string, std::map<std::string, std::string>> m_data;

    MatchConfig m_match;
    SpawnConfig m_spawn;
    WebSocketConfig m_websocket;
    ReviveConfig m_revive;
    WeaponsConfig m_weapons;
    CameraConfig m_camera;
    UIConfig m_ui;
};

} // namespace BattleGround
