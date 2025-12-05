#include "WebSocketClient.h"
#include "../config/ConfigManager.h"
#include "../utils/Logger.h"

#include <json.hpp>

// For Windows sockets
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#endif

#include <chrono>
#include <sstream>

using json = nlohmann::json;

namespace BattleGround {

WebSocketClient::WebSocketClient()
    : m_state(WebSocketState::DISCONNECTED)
    , m_autoReconnect(true)
    , m_reconnectInterval(5)
    , m_connectionTimeout(10)
    , m_running(false)
    , m_shouldReconnect(false)
    , m_lastReconnectAttempt(0)
{
}

WebSocketClient::~WebSocketClient() {
    Shutdown();
}

void WebSocketClient::Initialize() {
    const auto& config = ConfigManager::GetInstance().GetWebSocketConfig();
    m_serverUrl = config.serverURL;
    m_autoReconnect = config.autoReconnect;
    m_reconnectInterval = config.reconnectIntervalSeconds;
    m_connectionTimeout = config.connectionTimeout;

#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        LOG_ERROR("Failed to initialize Winsock");
        m_state = WebSocketState::ERROR;
        return;
    }
#endif

    LOG_INFO("WebSocket client initialized, server URL: " + m_serverUrl);
}

void WebSocketClient::Connect() {
    if (m_state == WebSocketState::CONNECTED || m_state == WebSocketState::CONNECTING) {
        return;
    }

    m_state = WebSocketState::CONNECTING;
    m_running = true;
    m_shouldReconnect = false;

    // Start connection in separate thread
    if (m_connectionThread.joinable()) {
        m_connectionThread.join();
    }
    m_connectionThread = std::thread(&WebSocketClient::ConnectionThread, this);

    LOG_INFO("WebSocket connecting to: " + m_serverUrl);
}

void WebSocketClient::Disconnect() {
    m_running = false;
    m_shouldReconnect = false;
    
    if (m_connectionThread.joinable()) {
        m_connectionThread.join();
    }

    m_state = WebSocketState::DISCONNECTED;
    LOG_INFO("WebSocket disconnected");
}

void WebSocketClient::Update() {
    // Check for reconnection
    if (m_shouldReconnect && m_autoReconnect) {
        auto now = std::chrono::system_clock::now().time_since_epoch().count() / 1000000000;
        if (now - m_lastReconnectAttempt >= m_reconnectInterval) {
            m_shouldReconnect = false;
            Connect();
        }
    }

    // Process callbacks for queued events
    if (m_giftCallback) {
        GiftEvent event;
        while (PopEvent(event)) {
            m_giftCallback(event);
        }
    }
}

void WebSocketClient::Shutdown() {
    Disconnect();

#ifdef _WIN32
    WSACleanup();
#endif

    LOG_INFO("WebSocket client shutdown");
}

void WebSocketClient::ConnectionThread() {
    // Simple WebSocket client implementation
    // In production, use a proper WebSocket library like websocketpp or beast
    
    // Parse URL to get host and port
    std::string url = m_serverUrl;
    std::string host;
    int port = 80;
    
    // Remove ws:// or wss:// prefix
    if (url.substr(0, 5) == "ws://") {
        url = url.substr(5);
    } else if (url.substr(0, 6) == "wss://") {
        url = url.substr(6);
        port = 443;
    }
    
    // Extract host and port
    size_t colonPos = url.find(':');
    size_t slashPos = url.find('/');
    
    if (colonPos != std::string::npos) {
        host = url.substr(0, colonPos);
        size_t portEnd = (slashPos != std::string::npos) ? slashPos : url.length();
        try {
            port = std::stoi(url.substr(colonPos + 1, portEnd - colonPos - 1));
        } catch (...) {
            port = 80;
        }
    } else {
        host = (slashPos != std::string::npos) ? url.substr(0, slashPos) : url;
    }

    LOG_INFO("Connecting to host: " + host + " port: " + std::to_string(port));

#ifdef _WIN32
    // Create socket
    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) {
        LOG_ERROR("Failed to create socket");
        m_state = WebSocketState::ERROR;
        ScheduleReconnect();
        return;
    }

    // Resolve host
    struct addrinfo hints = {}, *result = nullptr;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    if (getaddrinfo(host.c_str(), std::to_string(port).c_str(), &hints, &result) != 0) {
        LOG_ERROR("Failed to resolve host: " + host);
        closesocket(sock);
        m_state = WebSocketState::ERROR;
        ScheduleReconnect();
        return;
    }

    // Set socket timeout
    DWORD timeout = m_connectionTimeout * 1000;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (const char*)&timeout, sizeof(timeout));

    // Connect
    if (connect(sock, result->ai_addr, (int)result->ai_addrlen) == SOCKET_ERROR) {
        LOG_ERROR("Failed to connect to server");
        freeaddrinfo(result);
        closesocket(sock);
        m_state = WebSocketState::ERROR;
        ScheduleReconnect();
        return;
    }
    freeaddrinfo(result);

    // Send WebSocket handshake
    std::string path = (slashPos != std::string::npos) ? url.substr(slashPos) : "/";
    std::ostringstream handshake;
    handshake << "GET " << path << " HTTP/1.1\r\n"
              << "Host: " << host << ":" << port << "\r\n"
              << "Upgrade: websocket\r\n"
              << "Connection: Upgrade\r\n"
              << "Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\n"
              << "Sec-WebSocket-Version: 13\r\n"
              << "\r\n";

    std::string handshakeStr = handshake.str();
    if (send(sock, handshakeStr.c_str(), (int)handshakeStr.length(), 0) == SOCKET_ERROR) {
        LOG_ERROR("Failed to send handshake");
        closesocket(sock);
        m_state = WebSocketState::ERROR;
        ScheduleReconnect();
        return;
    }

    // Receive handshake response
    char buffer[4096];
    int bytesReceived = recv(sock, buffer, sizeof(buffer) - 1, 0);
    if (bytesReceived <= 0) {
        LOG_ERROR("Failed to receive handshake response");
        closesocket(sock);
        m_state = WebSocketState::ERROR;
        ScheduleReconnect();
        return;
    }
    buffer[bytesReceived] = '\0';

    // Check for successful upgrade
    std::string response(buffer);
    if (response.find("101") == std::string::npos) {
        LOG_ERROR("WebSocket upgrade failed");
        closesocket(sock);
        m_state = WebSocketState::ERROR;
        ScheduleReconnect();
        return;
    }

    m_state = WebSocketState::CONNECTED;
    LOG_INFO("WebSocket connected successfully");
    
    if (m_connectionCallback) {
        m_connectionCallback(true);
    }

    // Main receive loop
    while (m_running && m_state == WebSocketState::CONNECTED) {
        bytesReceived = recv(sock, buffer, sizeof(buffer) - 1, 0);
        
        if (bytesReceived <= 0) {
            if (m_running) {
                LOG_WARNING("Connection lost");
                m_state = WebSocketState::DISCONNECTED;
                ScheduleReconnect();
            }
            break;
        }

        // Parse WebSocket frame (simplified)
        if (bytesReceived >= 2) {
            // Get opcode and payload length
            unsigned char opcode = buffer[0] & 0x0F;
            unsigned char payloadLen = buffer[1] & 0x7F;
            
            // Text frame
            if (opcode == 0x01) {
                int offset = 2;
                int dataLen = payloadLen;
                
                if (payloadLen == 126 && bytesReceived >= 4) {
                    dataLen = (buffer[2] << 8) | buffer[3];
                    offset = 4;
                } else if (payloadLen == 127 && bytesReceived >= 10) {
                    // 64-bit length (we'll just use lower 32 bits)
                    dataLen = (buffer[6] << 24) | (buffer[7] << 16) | (buffer[8] << 8) | buffer[9];
                    offset = 10;
                }
                
                if (offset + dataLen <= bytesReceived) {
                    std::string message(buffer + offset, dataLen);
                    ProcessMessage(message);
                }
            }
            // Close frame
            else if (opcode == 0x08) {
                LOG_INFO("Server requested close");
                m_state = WebSocketState::DISCONNECTED;
                ScheduleReconnect();
                break;
            }
            // Ping frame - respond with pong
            else if (opcode == 0x09) {
                buffer[0] = (buffer[0] & 0xF0) | 0x0A; // Change to pong
                send(sock, buffer, bytesReceived, 0);
            }
        }
    }

    closesocket(sock);
    
    if (m_connectionCallback) {
        m_connectionCallback(false);
    }
#else
    // Non-Windows placeholder
    LOG_ERROR("WebSocket not implemented for this platform");
    m_state = WebSocketState::ERROR;
#endif
}

void WebSocketClient::ProcessMessage(const std::string& message) {
    try {
        json j = json::parse(message);
        
        if (j.contains("type") && j["type"] == "gift") {
            GiftEvent event;
            
            auto& data = j["data"];
            event.giftId = data.value("giftId", "");
            event.giftName = data.value("giftName", "");
            event.username = data.value("username", "");
            event.oderId = data.value("userId", "");
            event.quantity = data.value("quantity", 1);
            event.timestamp = data.value("timestamp", 0);
            event.diamondCost = data.value("diamondCost", 0);

            LOG_INFO("Gift received: " + event.giftName + " from " + event.username);
            QueueEvent(event);
        }
    } catch (const json::exception& e) {
        LOG_ERROR("Failed to parse message: " + std::string(e.what()));
    }
}

void WebSocketClient::QueueEvent(const GiftEvent& event) {
    std::lock_guard<std::mutex> lock(m_queueMutex);
    m_eventQueue.push(event);
}

bool WebSocketClient::HasPendingEvents() const {
    std::lock_guard<std::mutex> lock(m_queueMutex);
    return !m_eventQueue.empty();
}

bool WebSocketClient::PopEvent(GiftEvent& outEvent) {
    std::lock_guard<std::mutex> lock(m_queueMutex);
    if (m_eventQueue.empty()) {
        return false;
    }
    outEvent = m_eventQueue.front();
    m_eventQueue.pop();
    return true;
}

void WebSocketClient::ScheduleReconnect() {
    if (m_autoReconnect && m_running) {
        m_lastReconnectAttempt = std::chrono::system_clock::now().time_since_epoch().count() / 1000000000;
        m_shouldReconnect = true;
        m_state = WebSocketState::RECONNECTING;
        LOG_INFO("Scheduled reconnection in " + std::to_string(m_reconnectInterval) + " seconds");
    }
}

} // namespace BattleGround
