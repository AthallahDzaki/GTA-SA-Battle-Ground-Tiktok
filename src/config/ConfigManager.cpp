#include "ConfigManager.h"
#include "../utils/Logger.h"
#include <fstream>
#include <algorithm>

namespace BattleGround {

bool ConfigManager::Load(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        LOG_ERROR("Failed to open config file: " + filename);
        return false;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    ParseIniFile(buffer.str());

    // Load Match config
    m_match.minNPCsToStart = GetInt("Match", "MinNPCsToStart", 2);
    m_match.countdownSeconds = GetInt("Match", "CountdownSeconds", 5);
    m_match.winnerDelaySeconds = GetInt("Match", "WinnerDelaySeconds", 10);
    m_match.autoStartMatch = GetBool("Match", "AutoStartMatch", true);

    // Load Spawn config
    m_spawn.centerX = GetFloat("Spawn", "CenterX", 2488.0f);
    m_spawn.centerY = GetFloat("Spawn", "CenterY", -1666.0f);
    m_spawn.centerZ = GetFloat("Spawn", "CenterZ", 13.5f);
    m_spawn.spawnRadius = GetFloat("Spawn", "SpawnRadius", 50.0f);
    m_spawn.minHeight = GetFloat("Spawn", "MinHeight", 10.0f);
    m_spawn.maxHeight = GetFloat("Spawn", "MaxHeight", 20.0f);
    m_spawn.oneNPCPerUsername = GetBool("Spawn", "OneNPCPerUsername", true);
    m_spawn.joinGiftIds = GetStringList("Spawn", "JoinGiftIds", {"rose"});

    // Load WebSocket config
    m_websocket.serverURL = GetValue("WebSocket", "ServerURL", "ws://localhost:8080");
    m_websocket.autoReconnect = GetBool("WebSocket", "AutoReconnect", true);
    m_websocket.reconnectIntervalSeconds = GetInt("WebSocket", "ReconnectIntervalSeconds", 5);
    m_websocket.connectionTimeout = GetInt("WebSocket", "ConnectionTimeout", 10);

    // Load Revive config
    m_revive.reviveGiftIds = GetStringList("Revive", "ReviveGiftIds", {"diamond", "rose_gold", "galaxy"});
    m_revive.reviveHealthPercent = GetInt("Revive", "ReviveHealthPercent", 50);
    m_revive.invulnerabilitySeconds = GetInt("Revive", "InvulnerabilitySeconds", 3);
    m_revive.requireOwnerMatch = GetBool("Revive", "RequireOwnerMatch", true);

    // Load Weapons config
    m_weapons.defaultWeaponId = GetInt("Weapons", "DefaultWeaponId", 22);
    m_weapons.useRandomWeapons = GetBool("Weapons", "UseRandomWeapons", true);
    m_weapons.weaponList = GetIntList("Weapons", "WeaponList", {22, 24, 25, 28, 29, 30, 31});

    // Load Camera config
    m_camera.moveSpeed = GetFloat("Camera", "MoveSpeed", 1.0f);
    m_camera.fastSpeedMultiplier = GetFloat("Camera", "FastSpeedMultiplier", 3.0f);
    m_camera.slowSpeedMultiplier = GetFloat("Camera", "SlowSpeedMultiplier", 0.3f);
    m_camera.mouseSensitivity = GetFloat("Camera", "MouseSensitivity", 0.002f);
    m_camera.autoFollowEnabled = GetBool("Camera", "AutoFollowEnabled", false);
    m_camera.autoFollowDistance = GetFloat("Camera", "AutoFollowDistance", 15.0f);

    // Load UI config
    m_ui.showNPCNames = GetBool("UI", "ShowNPCNames", true);
    m_ui.showHealthBars = GetBool("UI", "ShowHealthBars", true);
    m_ui.showKillCount = GetBool("UI", "ShowKillCount", true);
    m_ui.showMatchStatus = GetBool("UI", "ShowMatchStatus", true);
    m_ui.notificationDuration = GetFloat("UI", "NotificationDuration", 5.0f);
    m_ui.showGiftNotifications = GetBool("UI", "ShowGiftNotifications", true);

    LOG_INFO("Configuration loaded successfully from: " + filename);
    return true;
}

bool ConfigManager::Save(const std::string& filename) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        LOG_ERROR("Failed to save config file: " + filename);
        return false;
    }

    // Write all sections
    for (const auto& section : m_data) {
        file << "[" << section.first << "]\n";
        for (const auto& kv : section.second) {
            file << kv.first << "=" << kv.second << "\n";
        }
        file << "\n";
    }

    LOG_INFO("Configuration saved to: " + filename);
    return true;
}

void ConfigManager::ParseIniFile(const std::string& content) {
    m_data.clear();
    std::istringstream stream(content);
    std::string line;
    std::string currentSection;

    while (std::getline(stream, line)) {
        line = Trim(line);
        
        // Skip empty lines and comments
        if (line.empty() || line[0] == ';' || line[0] == '#') {
            continue;
        }

        // Section header
        if (line[0] == '[' && line.back() == ']') {
            currentSection = line.substr(1, line.size() - 2);
            continue;
        }

        // Key-value pair
        size_t eqPos = line.find('=');
        if (eqPos != std::string::npos) {
            std::string key = Trim(line.substr(0, eqPos));
            std::string value = Trim(line.substr(eqPos + 1));
            m_data[currentSection][key] = value;
        }
    }
}

std::string ConfigManager::Trim(const std::string& str) {
    size_t start = str.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    size_t end = str.find_last_not_of(" \t\r\n");
    return str.substr(start, end - start + 1);
}

std::string ConfigManager::GetValue(const std::string& section, const std::string& key, const std::string& defaultValue) {
    auto sectionIt = m_data.find(section);
    if (sectionIt != m_data.end()) {
        auto keyIt = sectionIt->second.find(key);
        if (keyIt != sectionIt->second.end()) {
            return keyIt->second;
        }
    }
    return defaultValue;
}

int ConfigManager::GetInt(const std::string& section, const std::string& key, int defaultValue) {
    std::string value = GetValue(section, key, "");
    if (value.empty()) return defaultValue;
    try {
        return std::stoi(value);
    } catch (...) {
        return defaultValue;
    }
}

float ConfigManager::GetFloat(const std::string& section, const std::string& key, float defaultValue) {
    std::string value = GetValue(section, key, "");
    if (value.empty()) return defaultValue;
    try {
        return std::stof(value);
    } catch (...) {
        return defaultValue;
    }
}

bool ConfigManager::GetBool(const std::string& section, const std::string& key, bool defaultValue) {
    std::string value = GetValue(section, key, "");
    if (value.empty()) return defaultValue;
    std::transform(value.begin(), value.end(), value.begin(), ::tolower);
    return (value == "true" || value == "1" || value == "yes");
}

std::vector<std::string> ConfigManager::GetStringList(const std::string& section, const std::string& key, const std::vector<std::string>& defaultValue) {
    std::string value = GetValue(section, key, "");
    if (value.empty()) return defaultValue;
    
    std::vector<std::string> result;
    std::istringstream stream(value);
    std::string item;
    while (std::getline(stream, item, ',')) {
        item = Trim(item);
        if (!item.empty()) {
            result.push_back(item);
        }
    }
    return result.empty() ? defaultValue : result;
}

std::vector<int> ConfigManager::GetIntList(const std::string& section, const std::string& key, const std::vector<int>& defaultValue) {
    std::string value = GetValue(section, key, "");
    if (value.empty()) return defaultValue;
    
    std::vector<int> result;
    std::istringstream stream(value);
    std::string item;
    while (std::getline(stream, item, ',')) {
        item = Trim(item);
        if (!item.empty()) {
            try {
                result.push_back(std::stoi(item));
            } catch (...) {
                // Skip invalid values
            }
        }
    }
    return result.empty() ? defaultValue : result;
}

} // namespace BattleGround
