#pragma once

#include <string>
#include <functional>
#include <queue>
#include <mutex>
#include <thread>
#include <atomic>

namespace BattleGround {

// Gift data structure from TikTok
struct GiftEvent {
    std::string giftId;
    std::string giftName;
    std::string username;
    std::string oderId;
    int quantity;
    int64_t timestamp;
    int diamondCost;
};

// Connection state
enum class WebSocketState {
    DISCONNECTED,
    CONNECTING,
    CONNECTED,
    RECONNECTING,
    ERROR
};

class WebSocketClient {
public:
    static WebSocketClient& GetInstance() {
        static WebSocketClient instance;
        return instance;
    }

    // Lifecycle
    void Initialize();
    void Connect();
    void Disconnect();
    void Update();
    void Shutdown();

    // State
    WebSocketState GetState() const { return m_state; }
    bool IsConnected() const { return m_state == WebSocketState::CONNECTED; }

    // Event callbacks
    using GiftCallback = std::function<void(const GiftEvent&)>;
    using ConnectionCallback = std::function<void(bool connected)>;
    
    void SetGiftCallback(GiftCallback callback) { m_giftCallback = callback; }
    void SetConnectionCallback(ConnectionCallback callback) { m_connectionCallback = callback; }

    // Get queued events (thread-safe)
    bool HasPendingEvents() const;
    bool PopEvent(GiftEvent& outEvent);

private:
    WebSocketClient();
    ~WebSocketClient();
    WebSocketClient(const WebSocketClient&) = delete;
    WebSocketClient& operator=(const WebSocketClient&) = delete;

    void ConnectionThread();
    void ProcessMessage(const std::string& message);
    void QueueEvent(const GiftEvent& event);
    void ScheduleReconnect();

    std::atomic<WebSocketState> m_state;
    std::string m_serverUrl;
    bool m_autoReconnect;
    int m_reconnectInterval;
    int m_connectionTimeout;

    std::thread m_connectionThread;
    std::atomic<bool> m_running;
    std::atomic<bool> m_shouldReconnect;

    // Event queue (thread-safe)
    std::queue<GiftEvent> m_eventQueue;
    mutable std::mutex m_queueMutex;

    // Callbacks
    GiftCallback m_giftCallback;
    ConnectionCallback m_connectionCallback;

    // Reconnection timer
    int64_t m_lastReconnectAttempt;
};

} // namespace BattleGround
