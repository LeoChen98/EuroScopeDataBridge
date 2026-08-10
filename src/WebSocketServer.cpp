#include "WebSocketServer.h"
#include "Constants.h"

#include <nlohmann/json.hpp>

#include <iostream>
#include <sstream>
#include <algorithm>
#include <rpc.h>        // UuidCreate / UuidToStringA

using json = nlohmann::json;

namespace edb {

// ============================================================================
// GenerateClientId — UUID via Windows UuidCreate (unchanged from original)
// ============================================================================
std::string WebSocketServer::GenerateClientId() {
    UUID uuid;
    UuidCreate(&uuid);
    RPC_CSTR szUuid = nullptr;
    UuidToStringA(&uuid, &szUuid);
    std::string id(reinterpret_cast<const char*>(szUuid));
    RpcStringFreeA(&szUuid);
    return id;
}

// ============================================================================
// Construction / Destruction
// ============================================================================
WebSocketServer::WebSocketServer(int port, ThreadSafeQueue& incomingQueue)
    : m_port(port)
    , m_incomingQueue(incomingQueue)
{
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
}

WebSocketServer::~WebSocketServer() {
    Stop();
    WSACleanup();
}

// ============================================================================
// DrainIncomingQueue — called from EuroScope main thread (OnTimer)
// ============================================================================
void WebSocketServer::DrainIncomingQueue() {
    if (!m_requestProcessor) return;
    m_incomingQueue.Drain([this](std::string&& req) {
        m_requestProcessor(std::move(req));
    });
}

// ============================================================================
// Start
// ============================================================================
bool WebSocketServer::Start() {
    if (m_running.load())
        return false;

    try {
        m_server.init_asio();

        m_server.set_reuse_addr(true);

        // Keep error logging enabled for diagnostics
        m_server.clear_access_channels(websocketpp::log::alevel::all);
        m_server.set_error_channels(websocketpp::log::elevel::all);

        m_server.set_validate_handler(
            [this](ConnectionHdl hdl) {
                std::cout << "[DataBridge] OnValidate called" << std::endl;
                return OnValidate(hdl);
            });
        m_server.set_open_handler(
            [this](ConnectionHdl hdl) {
                std::cout << "[DataBridge] OnOpen called" << std::endl;
                OnOpen(hdl);
            });
        m_server.set_close_handler(
            [this](ConnectionHdl hdl) { OnClose(hdl); });
        m_server.set_message_handler(
            [this](ConnectionHdl hdl, WsServer::message_ptr msg) {
                OnMessage(hdl, msg);
            });
        m_server.set_fail_handler(
            [this](ConnectionHdl hdl) { OnFail(hdl); });

        // Listen on 127.0.0.1 + start async accept
        m_server.listen("127.0.0.1", std::to_string(m_port));
        m_server.start_accept();
    } catch (const std::exception& e) {
        std::ostringstream oss;
        oss << "[DataBridge] Failed to init WebSocket server: " << e.what();
        LogError(oss.str());
        return false;
    }

    // Start ASIO event loop in a dedicated thread
    m_running.store(true);
    m_ioThread = std::thread([this]() {
        std::cout << "[DataBridge] ASIO io_thread started, entering run()..." << std::endl;
        try {
            m_server.run();
        } catch (const std::exception& e) {
            std::cerr << "[DataBridge] ASIO run() exception: " << e.what() << std::endl;
        }
        std::cout << "[DataBridge] ASIO run() returned" << std::endl;
    });

    {
        std::ostringstream oss;
        oss << "[DataBridge] WebSocket server listening on ws://127.0.0.1:" << m_port;
        LogError(oss.str());
    }
    return true;
}

// ============================================================================
// Stop
// ============================================================================
void WebSocketServer::Stop() {
    if (!m_running.load())
        return;

    m_running.store(false);

    websocketpp::lib::error_code ec;
    m_server.stop_listening(ec);

    {
        std::lock_guard<std::mutex> lock(m_clientsMutex);
        for (auto& pair : m_sessions) {
            websocketpp::lib::error_code closeEc;
            m_server.close(pair.first, websocketpp::close::status::going_away, "Server shutting down", closeEc);
        }
    }

    m_server.stop();

    if (m_ioThread.joinable())
        m_ioThread.join();

    {
        std::lock_guard<std::mutex> lock(m_clientsMutex);
        m_sessions.clear();
        m_clientIdToHdl.clear();
    }
}

// ============================================================================
// OnValidate — reject connections when MAX_CLIENTS reached
// ============================================================================
bool WebSocketServer::OnValidate(ConnectionHdl /*hdl*/) {
    std::lock_guard<std::mutex> lock(m_clientsMutex);
    if (static_cast<int>(m_sessions.size()) >= MAX_CLIENTS) {
        std::ostringstream oss;
        oss << "[DataBridge] Max clients (" << MAX_CLIENTS << ") reached; rejecting new connection.";
        std::cout << oss.str() << std::endl;
        return false;
    }
    return true;
}

// ============================================================================
// OnOpen — allocate clientId and register session
// ============================================================================
void WebSocketServer::OnOpen(ConnectionHdl hdl) {
    std::string clientId = GenerateClientId();
    auto session = std::make_shared<ClientSession>(clientId);

    {
        std::lock_guard<std::mutex> lock(m_clientsMutex);
        m_sessions[hdl] = session;
        m_clientIdToHdl[clientId] = hdl;
    }

    {
        std::ostringstream oss;
        oss << "[DataBridge] Client connected: " << clientId;
        std::cout << oss.str() << std::endl;
    }
}

// ============================================================================
// OnClose — clean up session
// ============================================================================
void WebSocketServer::OnClose(ConnectionHdl hdl) {
    std::lock_guard<std::mutex> lock(m_clientsMutex);
    auto it = m_sessions.find(hdl);
    if (it != m_sessions.end()) {
        {
            std::ostringstream oss;
            oss << "[DataBridge] Client disconnected: " << it->second->clientId;
            std::cout << oss.str() << std::endl;
        }
        m_clientIdToHdl.erase(it->second->clientId);
        m_sessions.erase(it);
    }
}

// ============================================================================
// OnFail — clean up session (same as OnClose, connection failed)
// ============================================================================
void WebSocketServer::OnFail(ConnectionHdl hdl) {
    std::lock_guard<std::mutex> lock(m_clientsMutex);
    auto it = m_sessions.find(hdl);
    if (it != m_sessions.end()) {
        m_clientIdToHdl.erase(it->second->clientId);
        m_sessions.erase(it);
    }
}

// ============================================================================
// OnMessage — parse JSON, inject clientId, push to queue
// ============================================================================
void WebSocketServer::OnMessage(ConnectionHdl hdl, WsServer::message_ptr msg) {
    std::string clientId;
    {
        std::lock_guard<std::mutex> lock(m_clientsMutex);
        auto it = m_sessions.find(hdl);
        if (it == m_sessions.end()) return;
        clientId = it->second->clientId;
    }

    const std::string& payload = msg->get_payload();

    try {
        json j = json::parse(payload);
        if (!j.contains(json_key::TYPE)) {
            return;
        }

        j[json_key::CLIENT_ID] = clientId;
        m_incomingQueue.Push(j.dump());

    } catch (const json::parse_error&) {
        json err;
        err[json_key::TYPE] = msg_type::ERROR;
        err[json_key::DATA][json_key::ERROR] = "Invalid JSON";
        websocketpp::lib::error_code ec;
        m_server.send(hdl, err.dump(), websocketpp::frame::opcode::text, ec);
        return;
    }
}

// ============================================================================
// Broadcast — deliver to every handshake-complete client
// ============================================================================
void WebSocketServer::Broadcast(const std::string& msg) {
    std::lock_guard<std::mutex> lock(m_clientsMutex);
    for (auto& pair : m_sessions) {
        if (pair.second->handshakeDone) {
            websocketpp::lib::error_code ec;
            m_server.send(pair.first, msg, websocketpp::frame::opcode::text, ec);
        }
    }
}

// ============================================================================
// RouteOutgoing — deliver to a specific client, or broadcast when empty
// ============================================================================
void WebSocketServer::RouteOutgoing(const std::string& msg, const std::string& clientId) {
    if (clientId.empty()) {
        Broadcast(msg);
        return;
    }

    std::lock_guard<std::mutex> lock(m_clientsMutex);
    auto it = m_clientIdToHdl.find(clientId);
    if (it != m_clientIdToHdl.end()) {
        websocketpp::lib::error_code ec;
        m_server.send(it->second, msg, websocketpp::frame::opcode::text, ec);
    }
}

} // namespace edb
