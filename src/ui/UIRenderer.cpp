#include "UIRenderer.h"
#include "../npc/NPCManager.h"
#include "../match/MatchController.h"
#include "../config/ConfigManager.h"
#include "../utils/Logger.h"

// GTA SA SDK includes
#include <CFont.h>
#include <CSprite2d.h>
#include <CCamera.h>
#include <CDraw.h>
#include <CWorld.h>

namespace BattleGround {

const float UIRenderer::KILL_FEED_DURATION = 5.0f;

void UIRenderer::Initialize() {
    if (m_initialized) return;

    const auto& config = ConfigManager::GetInstance().GetUIConfig();
    m_showNPCNames = config.showNPCNames;
    m_showHealthBars = config.showHealthBars;
    m_showKillCount = config.showKillCount;
    m_showMatchStatus = config.showMatchStatus;
    m_notificationDuration = config.notificationDuration;

    m_initialized = true;
    LOG_INFO("UI Renderer initialized");
}

void UIRenderer::Update(float deltaTime) {
    // Update notification timers
    for (auto it = m_notifications.begin(); it != m_notifications.end();) {
        it->elapsed += deltaTime;
        if (it->elapsed >= it->duration) {
            it = m_notifications.erase(it);
        } else {
            ++it;
        }
    }

    // Update kill feed timers
    for (auto it = m_killFeed.begin(); it != m_killFeed.end();) {
        it->elapsed += deltaTime;
        if (it->elapsed >= KILL_FEED_DURATION) {
            it = m_killFeed.erase(it);
        } else {
            ++it;
        }
    }
}

void UIRenderer::Render() {
    if (!m_initialized) return;

    // Set up font for rendering
    CFont::SetBackground(false, false);
    CFont::SetProportional(true);
    CFont::SetJustify(false);
    CFont::SetOrientation(eFontAlignment::ALIGN_LEFT);
    CFont::SetFontStyle(eFontStyle::FONT_SUBTITLES);
    CFont::SetEdge(2);

    if (m_showMatchStatus) {
        RenderMatchStatus();
        RenderNPCCounter();
    }

    RenderNotifications();
    RenderKillFeed();

    if (m_showNPCNames) {
        RenderNPCNameTags();
    }

    if (m_showHealthBars) {
        RenderNPCHealthBars();
    }

    // Check for winner announcement
    auto* winner = MatchController::GetInstance().GetWinner();
    if (winner && MatchController::GetInstance().GetState() == MatchState::FINISHED) {
        RenderWinnerAnnouncement();
    }
}

void UIRenderer::Shutdown() {
    m_notifications.clear();
    m_killFeed.clear();
    m_initialized = false;
    LOG_INFO("UI Renderer shutdown");
}

void UIRenderer::AddNotification(NotificationType type, const std::string& message) {
    Notification notif;
    notif.type = type;
    notif.message = message;
    notif.duration = m_notificationDuration;
    notif.elapsed = 0;

    m_notifications.push_front(notif);
    
    // Limit queue size
    while (m_notifications.size() > MAX_NOTIFICATIONS) {
        m_notifications.pop_back();
    }

    LOG_DEBUG("Notification added: " + message);
}

void UIRenderer::AddKillFeedEntry(const std::string& killer, const std::string& victim) {
    KillFeedEntry entry;
    entry.killer = killer;
    entry.victim = victim;
    entry.elapsed = 0;

    m_killFeed.push_front(entry);
    
    // Limit queue size
    while (m_killFeed.size() > MAX_KILL_FEED) {
        m_killFeed.pop_back();
    }
}

void UIRenderer::RenderMatchStatus() {
    auto& match = MatchController::GetInstance();
    std::string status;
    unsigned int color = 0xFFFFFFFF;

    switch (match.GetState()) {
        case MatchState::WAITING:
            status = "Waiting for players...";
            color = 0xFFFFFF00; // Yellow
            break;
        case MatchState::COUNTDOWN:
            status = "Match starting in " + std::to_string(static_cast<int>(match.GetCountdown()) + 1) + "...";
            color = 0xFFFF8800; // Orange
            break;
        case MatchState::IN_PROGRESS:
            status = "MATCH IN PROGRESS";
            color = 0xFF00FF00; // Green
            break;
        case MatchState::WINNER_DELAY:
            status = "Deciding winner in " + std::to_string(static_cast<int>(match.GetWinnerDelay()) + 1) + "...";
            color = 0xFFFF0088; // Pink
            break;
        case MatchState::FINISHED:
            status = "MATCH FINISHED";
            color = 0xFF00FFFF; // Cyan
            break;
    }

    // Draw at top center
    float screenWidth = static_cast<float>(RsGlobal.maximumWidth);
    DrawText(screenWidth / 2.0f - 100.0f, 30.0f, status, color, 1.2f);
}

void UIRenderer::RenderNPCCounter() {
    auto& npcMgr = NPCManager::GetInstance();
    int alive = npcMgr.GetAliveNPCCount();
    int total = npcMgr.GetTotalNPCCount();

    std::string counter = "NPCs Alive: " + std::to_string(alive) + "/" + std::to_string(total);
    
    // Draw below match status
    float screenWidth = static_cast<float>(RsGlobal.maximumWidth);
    DrawText(screenWidth / 2.0f - 60.0f, 60.0f, counter, 0xFFFFFFFF, 1.0f);
}

void UIRenderer::RenderNotifications() {
    float y = 100.0f;
    float screenWidth = static_cast<float>(RsGlobal.maximumWidth);

    for (const auto& notif : m_notifications) {
        unsigned int color = 0xFFFFFFFF;
        
        switch (notif.type) {
            case NotificationType::GIFT_RECEIVED:
                color = 0xFF00FF00; // Green
                break;
            case NotificationType::SPAWN_SUCCESS:
                color = 0xFF00FF00; // Green
                break;
            case NotificationType::SPAWN_BLOCKED:
                color = 0xFFFF0000; // Red
                break;
            case NotificationType::NPC_DEATH:
                color = 0xFFFF4444; // Light red
                break;
            case NotificationType::REVIVE:
                color = 0xFFFFFF00; // Yellow
                break;
            case NotificationType::MATCH_START:
                color = 0xFF00FFFF; // Cyan
                break;
            case NotificationType::MATCH_END:
                color = 0xFF00FFFF; // Cyan
                break;
            case NotificationType::WINNER:
                color = 0xFFFFD700; // Gold
                break;
        }

        // Fade out effect
        float alpha = 1.0f - (notif.elapsed / notif.duration);
        if (alpha < 0) alpha = 0;
        
        // Apply alpha to color
        unsigned int finalColor = (color & 0x00FFFFFF) | (static_cast<unsigned int>(alpha * 255) << 24);
        
        DrawText(screenWidth - 400.0f, y, notif.message, finalColor, 0.8f);
        y += 25.0f;
    }
}

void UIRenderer::RenderKillFeed() {
    float y = 100.0f;

    for (const auto& entry : m_killFeed) {
        std::string text = entry.killer + " eliminated " + entry.victim;
        
        // Fade out effect
        float alpha = 1.0f - (entry.elapsed / KILL_FEED_DURATION);
        if (alpha < 0) alpha = 0;
        
        unsigned int color = 0xFFFFFFFF;
        color = (color & 0x00FFFFFF) | (static_cast<unsigned int>(alpha * 255) << 24);
        
        DrawText(20.0f, y, text, color, 0.7f);
        y += 20.0f;
    }
}

void UIRenderer::RenderNPCNameTags() {
    auto npcs = NPCManager::GetInstance().GetAliveNPCs();

    for (auto* npc : npcs) {
        if (!npc || !npc->IsAlive()) continue;

        Math::Vector3 pos = npc->GetPosition();
        float screenX, screenY;
        bool visible;
        
        WorldToScreen(pos.x, pos.y, pos.z + 1.5f, screenX, screenY, visible);
        
        if (visible) {
            DrawText(screenX - 30.0f, screenY - 30.0f, npc->GetUsername(), 0xFFFFFFFF, 0.6f);
        }
    }
}

void UIRenderer::RenderNPCHealthBars() {
    auto npcs = NPCManager::GetInstance().GetAliveNPCs();

    for (auto* npc : npcs) {
        if (!npc || !npc->IsAlive()) continue;

        Math::Vector3 pos = npc->GetPosition();
        float screenX, screenY;
        bool visible;
        
        WorldToScreen(pos.x, pos.y, pos.z + 1.3f, screenX, screenY, visible);
        
        if (visible) {
            float health = npc->GetHealth();
            float maxHealth = npc->GetMaxHealth();
            float healthPercent = health / maxHealth;
            
            // Background bar
            DrawBox(screenX - 25.0f, screenY - 15.0f, 50.0f, 6.0f, 0x80000000);
            
            // Health bar
            unsigned int healthColor = 0xFF00FF00;
            if (healthPercent < 0.3f) healthColor = 0xFFFF0000;
            else if (healthPercent < 0.6f) healthColor = 0xFFFFFF00;
            
            DrawBox(screenX - 24.0f, screenY - 14.0f, 48.0f * healthPercent, 4.0f, healthColor);
            
            // Invulnerability indicator
            if (npc->IsInvulnerable()) {
                DrawText(screenX - 20.0f, screenY - 25.0f, "INVULN", 0xFFFFFF00, 0.4f);
            }
        }
    }
}

void UIRenderer::RenderWinnerAnnouncement() {
    auto* winner = MatchController::GetInstance().GetWinner();
    if (!winner) return;

    float screenWidth = static_cast<float>(RsGlobal.maximumWidth);
    float screenHeight = static_cast<float>(RsGlobal.maximumHeight);

    std::string text = "WINNER: " + winner->GetUsername() + "!";
    
    // Large centered text
    CFont::SetScale(3.0f, 4.0f);
    CFont::SetColor(CRGBA(255, 215, 0, 255)); // Gold
    CFont::SetOrientation(eFontAlignment::ALIGN_CENTER);
    CFont::PrintString(screenWidth / 2.0f, screenHeight / 2.0f - 50.0f, text.c_str());
    
    // Kill count
    std::string kills = "Kills: " + std::to_string(winner->GetKillCount());
    CFont::SetScale(1.5f, 2.0f);
    CFont::SetColor(CRGBA(255, 255, 255, 255));
    CFont::PrintString(screenWidth / 2.0f, screenHeight / 2.0f + 20.0f, kills.c_str());
}

void UIRenderer::DrawText(float x, float y, const std::string& text, unsigned int color, float scale) {
    // Extract ARGB components
    unsigned char a = (color >> 24) & 0xFF;
    unsigned char r = (color >> 16) & 0xFF;
    unsigned char g = (color >> 8) & 0xFF;
    unsigned char b = color & 0xFF;

    CFont::SetScale(scale * 0.5f, scale);
    CFont::SetColor(CRGBA(r, g, b, a));
    CFont::SetOrientation(eFontAlignment::ALIGN_LEFT);
    CFont::PrintString(x, y, text.c_str());
}

void UIRenderer::DrawBox(float x, float y, float width, float height, unsigned int color) {
    // Extract ARGB components
    unsigned char a = (color >> 24) & 0xFF;
    unsigned char r = (color >> 16) & 0xFF;
    unsigned char g = (color >> 8) & 0xFF;
    unsigned char b = color & 0xFF;

    CSprite2d::DrawRect(CRect(x, y, x + width, y + height), CRGBA(r, g, b, a));
}

void UIRenderer::WorldToScreen(float worldX, float worldY, float worldZ, float& screenX, float& screenY, bool& visible) {
    CVector world(worldX, worldY, worldZ);
    CVector screen;
    
    // Use camera matrix to transform world to screen coordinates
    float w;
    visible = CSprite::CalcScreenCoors(world, &screen, &w, &w, true, true);
    
    if (visible) {
        screenX = screen.x;
        screenY = screen.y;
    }
}

} // namespace BattleGround
