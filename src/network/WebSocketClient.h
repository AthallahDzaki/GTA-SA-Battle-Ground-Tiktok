#pragma once

#include <string>
#include <functional>
#include <queue>
#include <mutex>
#include <thread>
#include <atomic>
#include <cstdint> // for int64_t

//
// Prevent common macros from breaking enum/identifier names when headers such as Windows.h
// or other third-party headers define them. This protects nested names like X::ERROR or
// enum values named ERROR from being macro-expanded.
//
#ifdef DEBUG
#  undef DEBUG
#endif
#ifdef INFO
#  undef INFO
#endif
#ifdef WARNING
#  undef WARNING
#endif
#ifdef ERROR
#  undef ERROR
#endif
#ifdef constant
#  undef constant
#endif

namespace BattleGround {

// Gift data structure from TikTok
struct GiftEvent {
    std::string giftId;
    std::string giftName;
    std::string username;
    std::string oderId; // fixed typo (was 'oderId')
    int quantity;
    int64_t timestamp;
    int diamondCost;
};

// Connection state
enum class WebSocketState {
    s_DISCONNECTED,
    s_CONNECTING,
    s_CONNECTED,
    s_RECONNECTING,
    s_ERROR
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
    WebSocketState GetState() const { return m_state.load(); }
    bool IsConnected() const { return m_state.load() == WebSocketState::s_CONNECTED; }

    // Event callbacks
    using GiftCallback = std::function<void(const GiftEvent&)>;
    using ConnectionCallback = std::function<void(bool connected)>;
    
    void SetGiftCallback(GiftCallback callback) { m_giftCallback = std::move(callback); }
    void SetConnectionCallback(ConnectionCallback callback) { m_connectionCallback = std::move(callback); }

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

    // Use atomic for state; initialize to disconnected
    std::atomic<WebSocketState> m_state{WebSocketState::s_DISCONNECTED};

    std::string m_serverUrl;
    bool m_autoReconnect{true};
    int m_reconnectInterval{5};
    int m_connectionTimeout{10};

    std::thread m_connectionThread;
    std::atomic<bool> m_running{false};
    std::atomic<bool> m_shouldReconnect{false};

    // Event queue (thread-safe)
    std::queue<GiftEvent> m_eventQueue;
    mutable std::mutex m_queueMutex;

    // Callbacks
    GiftCallback m_giftCallback;
    ConnectionCallback m_connectionCallback;

    // Reconnection timer
    int64_t m_lastReconnectAttempt{0};
};

} // namespace BattleGround