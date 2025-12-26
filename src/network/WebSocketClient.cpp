#pragma once

// Protect this translation unit from common macro collisions.
// These must be placed BEFORE any includes so macros don't break C++ tokens like Foo::constant.
#ifndef WIN32_LEAN_AND_MEAN
#  define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#  define NOMINMAX
#endif

#ifdef constant
#  undef constant
#endif
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
#include <vector>
#include <cstdint>
#include <cstddef>
#include <cstdlib>
#include <ctime>
#include <algorithm>
#include <cstring>

using json = nlohmann::json;

namespace BattleGround {

WebSocketClient::WebSocketClient()
    : m_state(WebSocketState::s_DISCONNECTED)
    , m_autoReconnect(true)
    , m_reconnectInterval(5)
    , m_connectionTimeout(10)
    , m_running(false)
    , m_shouldReconnect(false)
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
        m_state = WebSocketState::s_ERROR;
        return;
    }
#endif

    LOG_INFO("WebSocket client initialized, server URL: " + m_serverUrl);
}

void WebSocketClient::Connect() {
    if (m_state == WebSocketState::s_CONNECTED || m_state == WebSocketState::s_CONNECTING) {
        return;
    }

    m_state = WebSocketState::s_CONNECTING;
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

    m_state = WebSocketState::s_DISCONNECTED;
    LOG_INFO("WebSocket disconnected");
}

void WebSocketClient::Update() {
    // Check for reconnection
    if (m_shouldReconnect && m_autoReconnect) {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = now - m_lastReconnectAttempt; // duration

        LOG_DEBUG("Elapsed (s): " + std::to_string(
            std::chrono::duration_cast<std::chrono::seconds>(elapsed).count()
        ));
        if (elapsed >= std::chrono::seconds(m_reconnectInterval)) {
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

// parse header safe: returns true jika header lengkap. Untuk build 32-bit, dataLen dibatasi ke uint32_t.
static bool parse_websocket_header_safe(const char* buffer, size_t bytesReceived,
                                 uint32_t &dataLen, size_t &payloadOffset, bool &masked)
{
    if (bytesReceived < 2) return false;

    uint8_t b0 = static_cast<uint8_t>(buffer[0]);
    uint8_t b1 = static_cast<uint8_t>(buffer[1]);

    masked = (b1 & 0x80) != 0;
    uint8_t payloadLen = b1 & 0x7F;

    size_t index = 2;

    if (payloadLen <= 125) {
        dataLen = payloadLen;
    } else if (payloadLen == 126) {
        if (bytesReceived < index + 2) return false;
        uint8_t hi = static_cast<uint8_t>(buffer[index]);
        uint8_t lo = static_cast<uint8_t>(buffer[index + 1]);
        uint32_t len = (static_cast<uint32_t>(hi) << 8) | static_cast<uint32_t>(lo);
        dataLen = len;
        index += 2;
    } else { // payloadLen == 127
        // need 8 bytes; read big-endian. For 32-bit client, reject if > UINT32_MAX.
        if (bytesReceived < index + 8) return false;
        uint64_t len64 = 0;
        for (int i = 0; i < 8; ++i) {
            len64 = (len64 << 8) | static_cast<uint8_t>(buffer[index + i]);
        }
        index += 8;
        if (len64 > static_cast<uint64_t>(UINT32_MAX)) {
            // too large for 32-bit client - reject
            return false;
        }
        dataLen = static_cast<uint32_t>(len64);
    }

    if (masked) {
        if (bytesReceived < index + 4) return false;
        index += 4;
    }

    // safety cap (optional)
    const uint32_t MAX_ALLOWED = 1u << 30; // example 1 GiB cap
    if (dataLen > MAX_ALLOWED) return false;

    payloadOffset = index;
    return true;
}

// Append recv into vector buffer. Returns <=0 on close/error (same as recv).
static int recv_append(SOCKET s, std::vector<char>& buf)
{
    char tmp[4096];
    int r = recv(s, tmp, sizeof(tmp), 0);
    if (r > 0) {
        buf.insert(buf.end(), tmp, tmp + r);
    }
    return r;
}

// Send a websocket frame (client MUST mask frames sent to server).
// opcode: e.g., 0x01=text, 0x0A=pong, 0x08=close
static bool send_ws_frame(SOCKET s, uint8_t opcode, const std::vector<char>& payload)
{
    std::vector<char> out;
    out.reserve(14 + payload.size());

    uint8_t fin_and_opcode = 0x80 | (opcode & 0x0F);
    out.push_back(static_cast<char>(fin_and_opcode));

    // client-to-server MUST set mask bit
    uint64_t len = payload.size();
    if (len <= 125) {
        out.push_back(static_cast<char>(0x80 | static_cast<uint8_t>(len)));
    } else if (len <= 0xFFFF) {
        out.push_back(static_cast<char>(0x80 | 126));
        out.push_back(static_cast<char>((len >> 8) & 0xFF));
        out.push_back(static_cast<char>(len & 0xFF));
    } else {
        out.push_back(static_cast<char>(0x80 | 127));
        // 64-bit length big-endian
        for (int i = 7; i >= 0; --i) {
            out.push_back(static_cast<char>((len >> (8 * i)) & 0xFF));
        }
    }

    // generate mask key
    uint8_t maskKey[4];
    srand(static_cast<unsigned int>(time(NULL)) ^ GetCurrentThreadId());
    for (int i = 0; i < 4; ++i) maskKey[i] = static_cast<uint8_t>(rand() & 0xFF);

    out.push_back(static_cast<char>(maskKey[0]));
    out.push_back(static_cast<char>(maskKey[1]));
    out.push_back(static_cast<char>(maskKey[2]));
    out.push_back(static_cast<char>(maskKey[3]));

    // masked payload
    for (size_t i = 0; i < payload.size(); ++i) {
        uint8_t maskedByte = static_cast<uint8_t>(payload[i]) ^ maskKey[i % 4];
        out.push_back(static_cast<char>(maskedByte));
    }

    int sent = send(s, out.data(), (int)out.size(), 0);
    return (sent == (int)out.size());
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
        m_state = WebSocketState::s_ERROR;
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
        m_state = WebSocketState::s_ERROR;
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
        m_state = WebSocketState::s_ERROR;
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
        m_state = WebSocketState::s_ERROR;
        ScheduleReconnect();
        return;
    }

    // Receive handshake response (read until header end or timeout)
    char tmpBuf[4096];
    int bytesReceived = recv(sock, tmpBuf, (int)sizeof(tmpBuf) - 1, 0);
    if (bytesReceived <= 0) {
        LOG_ERROR("Failed to receive handshake response");
        closesocket(sock);
        m_state = WebSocketState::s_ERROR;
        ScheduleReconnect();
        return;
    }
    tmpBuf[bytesReceived] = '\0';

    // Check for successful upgrade
    std::string response(tmpBuf);
    if (response.find("101") == std::string::npos) {
        LOG_ERROR("WebSocket upgrade failed");
        closesocket(sock);
        m_state = WebSocketState::s_ERROR;
        ScheduleReconnect();
        return;
    }

    m_state = WebSocketState::s_CONNECTED;
    LOG_INFO("WebSocket connected successfully");

    if (m_connectionCallback) {
        m_connectionCallback(true);
    }

    // Main receive loop
    {
        std::vector<char> buf;
        buf.reserve(8192);
        bool exitLoop = false;

        while (m_running && m_state == WebSocketState::s_CONNECTED && !exitLoop) {
            int r = recv_append(sock, buf);
            if (r == 0) {
                // orderly close from server
                LOG_INFO("Connection closed by remote");
                m_state = WebSocketState::s_DISCONNECTED;
                ScheduleReconnect();
                break;
            } else if (r < 0) {
                int err = WSAGetLastError();
                if (err == WSAETIMEDOUT) {
                    // timeout is expected occasionally; check m_running and continue
                    continue;
                }
                LOG_WARNING("recv failed, error: " + std::to_string(err));
                m_state = WebSocketState::s_DISCONNECTED;
                ScheduleReconnect();
                break;
            }

            // Try to parse as many frames as available in buffer
            while (true) {
                uint32_t dataLen = 0;
                size_t payloadOffset = 0;
                bool masked = false;

                bool header_ok = parse_websocket_header_safe(buf.data(), buf.size(), dataLen, payloadOffset, masked);
                if (!header_ok) {
                    // header incomplete - wait for more data
                    break;
                }

                size_t totalNeeded = payloadOffset + static_cast<size_t>(dataLen);
                if (buf.size() < totalNeeded) {
                    // payload incomplete - wait for more data
                    break;
                }

                // Safe to extract opcode and handle frame
                uint8_t b0 = static_cast<uint8_t>(buf[0]);
                uint8_t opcode = b0 & 0x0F;

                // Extract payload into vector
                std::vector<char> payload;
                if (dataLen > 0) {
                    payload.resize(dataLen);
                    memcpy(payload.data(), buf.data() + payloadOffset, dataLen);
                }

                // If masked, unmask (mask key positioned at payloadOffset - 4)
                if (masked) {
                    if (payloadOffset >= 4) {
                        const uint8_t* maskKey = reinterpret_cast<const uint8_t*>(buf.data() + (payloadOffset - 4));
                        for (uint32_t i = 0; i < dataLen; ++i) {
                            payload[i] = static_cast<char>( static_cast<uint8_t>(payload[i]) ^ maskKey[i % 4] );
                        }
                    } else {
                        LOG_WARNING("Malformed masked frame (mask key not present)");
                    }
                }

                // Handle opcodes
                if (opcode == 0x01) { // text
                    std::string message(payload.begin(), payload.end());
                    ProcessMessage(message);
                } else if (opcode == 0x08) { // close
                    LOG_INFO("Server requested close");
                    m_state = WebSocketState::s_DISCONNECTED;
                    ScheduleReconnect();
                    // consume and exit
                    buf.erase(buf.begin(), buf.begin() + totalNeeded);
                    exitLoop = true;
                    break;
                } else if (opcode == 0x09) { // ping: reply with pong, same payload
                    bool ok = send_ws_frame(sock, 0x0A, payload); // client MUST mask
                    if (!ok) {
                        LOG_WARNING("Failed to send pong");
                    }
                } else if (opcode == 0x0A) { // pong - ignore or log
                    // optional: handle keepalive logic
                } else {
                    // other opcodes: continuation, binary, etc. ignore or implement as needed
                }

                // consume processed bytes from buffer and continue parsing possible next frames
                buf.erase(buf.begin(), buf.begin() + totalNeeded);
            } // inner parsing loop
        } // main loop

        // cleanup: close socket
        closesocket(sock);

        if (m_connectionCallback) {
            m_connectionCallback(false);
        }
    }

#else
    // Non-Windows placeholder
    LOG_ERROR("WebSocket not implemented for this platform");
    m_state = WebSocketState::s_ERROR;
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
            event.oderId = data.value("userId", ""); // updated to userId
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
        m_lastReconnectAttempt = std::chrono::steady_clock::now();
        m_shouldReconnect = true;
        m_state = WebSocketState::s_RECONNECTING;
        LOG_INFO("Scheduled reconnection in " + std::to_string(m_reconnectInterval) + " seconds");
    }
}

} // namespace BattleGround