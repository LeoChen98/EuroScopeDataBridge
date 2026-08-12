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
        m_subscriptionCounts.clear();
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
        // Remove this client's subscriptions from the global counts
        for (const auto& e : it->second->subscriptions) {
            auto cnt = m_subscriptionCounts.find(e);
            if (cnt != m_subscriptionCounts.end() && --cnt->second == 0)
                m_subscriptionCounts.erase(cnt);
        }
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
        for (const auto& e : it->second->subscriptions) {
            auto cnt = m_subscriptionCounts.find(e);
            if (cnt != m_subscriptionCounts.end() && --cnt->second == 0)
                m_subscriptionCounts.erase(cnt);
        }
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
// Broadcast — deliver a push event only to clients subscribed to its type
// ============================================================================
void WebSocketServer::Broadcast(const std::string& type, const std::string& msg) {
    std::lock_guard<std::mutex> lock(m_clientsMutex);
    for (auto& pair : m_sessions) {
        if (pair.second->handshakeDone && pair.second->subscriptions.count(type) > 0) {
            websocketpp::lib::error_code ec;
            m_server.send(pair.first, msg, websocketpp::frame::opcode::text, ec);
        }
    }
}

// ============================================================================
// HasSubscribers — whether any client subscribed to the event type
// ============================================================================
bool WebSocketServer::HasSubscribers(const std::string& type) {
    std::lock_guard<std::mutex> lock(m_clientsMutex);
    auto it = m_subscriptionCounts.find(type);
    return it != m_subscriptionCounts.end() && it->second > 0;
}

// ============================================================================
// HandleSubscriptionRequest — process subscribe / unsubscribe (ES main thread)
// ============================================================================
std::string WebSocketServer::HandleSubscriptionRequest(const std::string& requestJson)
{
    json req;
    try {
        req = json::parse(requestJson);
    } catch (const json::parse_error&) {
        return "";
    }

    std::string id;
    auto idIt = req.find(json_key::ID);
    if (idIt != req.end() && idIt->is_string())
        id = idIt->get<std::string>();

    std::string type;
    auto typeIt = req.find(json_key::TYPE);
    if (typeIt != req.end() && typeIt->is_string())
        type = typeIt->get<std::string>();

    std::string clientId;
    auto clientIt = req.find(json_key::CLIENT_ID);
    if (clientIt != req.end() && clientIt->is_string())
        clientId = clientIt->get<std::string>();

    auto makeResponse = [&id](bool success, const std::string& error, const std::set<std::string>& subs) {
        json resp;
        resp[json_key::TYPE] = msg_type::RESPONSE;
        resp[json_key::ID] = id;
        resp[json_key::DATA][json_key::SUCCESS] = success;
        if (!error.empty())
            resp[json_key::DATA][json_key::ERROR] = error;
        if (success) {
            resp[json_key::DATA][json_key::EVENTS] = json::array();
            for (const auto& e : subs)
                resp[json_key::DATA][json_key::EVENTS].push_back(e);
        }
        return resp.dump();
    };

    // Collect requested event types
    std::set<std::string> events;
    if (req.contains(json_key::DATA) && req[json_key::DATA].is_object())
    {
        auto evIt = req[json_key::DATA].find(json_key::EVENTS);
        if (evIt != req[json_key::DATA].end())
        {
            if (!evIt->is_array())
                return makeResponse(false, "Invalid 'events' field; expected an array of strings", events);
            for (const auto& e : *evIt)
                if (e.is_string())
                    events.insert(e.get<std::string>());
        }
    }

    const bool subscribe = (type == msg_type::SUBSCRIBE);

    std::lock_guard<std::mutex> lock(m_clientsMutex);

    auto it = m_clientIdToHdl.find(clientId);
    if (it == m_clientIdToHdl.end())
        return makeResponse(false, "Unknown client", events);

    auto sessionIt = m_sessions.find(it->second);
    if (sessionIt == m_sessions.end())
        return makeResponse(false, "Unknown client", events);

    auto& subs = sessionIt->second->subscriptions;

    if (subscribe)
    {
        for (const auto& e : events)
        {
            if (subs.insert(e).second)
                ++m_subscriptionCounts[e];
        }
    }
    else
    {
        // unsubscribe: no events given (or empty array) clears all subscriptions
        if (events.empty())
        {
            for (const auto& e : subs)
            {
                auto cnt = m_subscriptionCounts.find(e);
                if (cnt != m_subscriptionCounts.end() && --cnt->second == 0)
                    m_subscriptionCounts.erase(cnt);
            }
            subs.clear();
        }
        else
        {
            for (const auto& e : events)
            {
                if (subs.erase(e) > 0)
                {
                    auto cnt = m_subscriptionCounts.find(e);
                    if (cnt != m_subscriptionCounts.end() && --cnt->second == 0)
                        m_subscriptionCounts.erase(cnt);
                }
            }
        }
    }

    return makeResponse(true, "", subs);
}

// ============================================================================
// RouteOutgoing — deliver to a specific client, or broadcast when empty
// ============================================================================
void WebSocketServer::RouteOutgoing(const std::string& msg, const std::string& clientId) {
    if (clientId.empty()) {
        std::lock_guard<std::mutex> lock(m_clientsMutex);
        SendToAllLocked(msg);
        return;
    }

    std::lock_guard<std::mutex> lock(m_clientsMutex);
    auto it = m_clientIdToHdl.find(clientId);
    if (it != m_clientIdToHdl.end()) {
        websocketpp::lib::error_code ec;
        m_server.send(it->second, msg, websocketpp::frame::opcode::text, ec);
    }
}

// ============================================================================
// SendToAllLocked — send to every handshake-complete client (no filter)
// ============================================================================
void WebSocketServer::SendToAllLocked(const std::string& msg) {
    for (auto& pair : m_sessions) {
        if (pair.second->handshakeDone) {
            websocketpp::lib::error_code ec;
            m_server.send(pair.first, msg, websocketpp::frame::opcode::text, ec);
        }
    }
}

} // namespace edb
