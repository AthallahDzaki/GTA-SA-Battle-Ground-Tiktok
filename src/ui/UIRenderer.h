#pragma once

#include "../npc/BattleNPC.h"
#include <string>
#include <vector>
#include <deque>

namespace BattleGround {

// Notification types for different UI styles
enum class NotificationType {
    GIFT_RECEIVED,
    SPAWN_SUCCESS,
    SPAWN_BLOCKED,  // Username already has NPC
    NPC_DEATH,
    REVIVE,
    MATCH_START,
    MATCH_END,
    WINNER
};

struct Notification {
    NotificationType type;
    std::string message;
    float duration;
    float elapsed;
};

struct KillFeedEntry {
    std::string killer;
    std::string victim;
    float elapsed;
};

class UIRenderer {
public:
    static UIRenderer& GetInstance() {
        static UIRenderer instance;
        return instance;
    }

    void Initialize();
    void Update(float deltaTime);
    void Render();
    void Shutdown();

    // Notifications
    void AddNotification(NotificationType type, const std::string& message);
    void AddKillFeedEntry(const std::string& killer, const std::string& victim);

    // Display control
    void SetShowNPCNames(bool show) { m_showNPCNames = show; }
    void SetShowHealthBars(bool show) { m_showHealthBars = show; }
    void SetShowKillCount(bool show) { m_showKillCount = show; }
    void SetShowMatchStatus(bool show) { m_showMatchStatus = show; }

private:
    UIRenderer() = default;
    ~UIRenderer() = default;
    UIRenderer(const UIRenderer&) = delete;
    UIRenderer& operator=(const UIRenderer&) = delete;

    void RenderMatchStatus();
    void RenderNPCCounter();
    void RenderNotifications();
    void RenderKillFeed();
    void RenderNPCNameTags();
    void RenderNPCHealthBars();
    void RenderWinnerAnnouncement();

    void RenderText(float x, float y, const std::string& text, unsigned int color = 0xFFFFFFFF, float scale = 1.0f);
    void DrawBox(float x, float y, float width, float height, unsigned int color);
    void WorldToScreen(float worldX, float worldY, float worldZ, float& screenX, float& screenY, bool& visible);

    // Notifications queue
    std::deque<Notification> m_notifications;
    std::deque<KillFeedEntry> m_killFeed;
    
    // Config
    bool m_showNPCNames = true;
    bool m_showHealthBars = true;
    bool m_showKillCount = true;
    bool m_showMatchStatus = true;
    float m_notificationDuration = 5.0f;
    
    static const int MAX_NOTIFICATIONS = 5;
    static const int MAX_KILL_FEED = 5;
    static const float KILL_FEED_DURATION;

    bool m_initialized = false;
};

} // namespace BattleGround
