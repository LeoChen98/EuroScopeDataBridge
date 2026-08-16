#include "WebSocketServer.h"
#include "Constants.h"

#include <nlohmann/json.hpp>

#include <iostream>
#include <sstream>
#include <algorithm>
#include <vector>
#include <chrono>
#include <rpc.h>        // UuidCreate / UuidToStringA

using json = nlohmann::json;

namespace edb {

// ============================================================================
// GenerateClientId — UUID via Windows UuidCreate, with a safe fallback
// ============================================================================
std::string WebSocketServer::GenerateClientId() {
    static std::atomic<unsigned long long> fallbackCounter{0};

    UUID uuid;
    RPC_CSTR szUuid = nullptr;
    if (UuidCreate(&uuid) == RPC_S_OK &&
        UuidToStringA(&uuid, &szUuid) == RPC_S_OK &&
        szUuid != nullptr)
    {
        std::string id(reinterpret_cast<const char*>(szUuid));
        RpcStringFreeA(&szUuid);
        return id;
    }
    if (szUuid != nullptr)
        RpcStringFreeA(&szUuid);

    // Fallback: 100 ns file time + monotonic counter (finer resolution than
    // the 1 s wall clock, keeps ids unique under rapid reconnect bursts).
    FILETIME ft;
    GetSystemTimeAsFileTime(&ft);
    const unsigned long long t =
        (static_cast<unsigned long long>(ft.dwHighDateTime) << 32) | ft.dwLowDateTime;
    char buf[96];
    snprintf(buf, sizeof(buf), "%llu-%llu", t,
             static_cast<unsigned long long>(fallbackCounter.fetch_add(1)));
    return std::string(buf);
}

// ============================================================================
// Construction / Destruction
// ============================================================================
WebSocketServer::WebSocketServer(int port, ThreadSafeQueue& incomingQueue)
    : m_port(port)
    , m_incomingQueue(incomingQueue)
{
    WSADATA wsaData;
    const int err = WSAStartup(MAKEWORD(2, 2), &wsaData);
    m_wsaInitialized = (err == 0);
    if (!m_wsaInitialized)
        std::cerr << "[DataBridge] WSAStartup failed with error " << err << std::endl;
}

WebSocketServer::~WebSocketServer() {
    Stop();
    if (m_wsaInitialized)
        WSACleanup();
}

// ============================================================================
// DrainIncomingQueue — called from the EuroScope main thread (OnTimer)
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
        if (!m_asioInitialized) {
            // First run: the asio transport is UNINITIALIZED and holds a null
            // io_service, so reset() must not be called yet — it would
            // dereference the null pointer inside io_context::restart().
            m_server.init_asio();
            m_asioInitialized = true;
        } else {
            // Reuse after Stop(): the transport stays in READY state and
            // init_asio() may not be called again ("asio::init_asio called
            // from the wrong state"), but Stop() halted the io_service, so
            // restart it before listen(). reset() is safe here: the
            // io_service exists.
            m_server.reset();
        }

        m_server.set_reuse_addr(true);

        // Reject oversized frames before they are buffered (memory guard).
        m_server.set_max_message_size(MAX_INCOMING_MESSAGE_BYTES);

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
            m_running.store(false);
        } catch (...) {
            std::cerr << "[DataBridge] ASIO run() unknown exception" << std::endl;
            m_running.store(false);
        }
        // run() only returns normally after stop(); if it returns while the
        // server is still marked running, the IO loop died unexpectedly.
        if (m_running.exchange(false)) {
            std::cerr << "[DataBridge] ASIO run() returned unexpectedly; server stopped." << std::endl;
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
    if (m_running.load()) {
        m_running.store(false);

        websocketpp::lib::error_code ec;
        m_server.stop_listening(ec);

        // Collect the handles under the lock, then close them without holding it —
        // avoids any lock-order inversion with IO-thread close/fail handlers.
        std::vector<ConnectionHdl> hdls;
        {
            std::lock_guard<std::mutex> lock(m_clientsMutex);
            hdls.reserve(m_sessions.size());
            for (auto& pair : m_sessions) {
                hdls.push_back(pair.first);
            }
        }
        for (auto& hdl : hdls) {
            websocketpp::lib::error_code closeEc;
            m_server.close(hdl, websocketpp::close::status::going_away, "Server shutting down", closeEc);
        }

        m_server.stop();
    }

    // Always join the IO thread: it may have exited early (run() threw) after
    // setting m_running=false itself. Skipping join in that case would leave
    // the thread joinable, and ~std::thread would call std::terminate.
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

    auto sendError = [this, &hdl](const char* message) {
        json err;
        err[json_key::TYPE] = msg_type::ERROR;
        err[json_key::DATA][json_key::ERROR] = message;
        websocketpp::lib::error_code ec;
        m_server.send(hdl, err.dump(), websocketpp::frame::opcode::text, ec);
    };

    json j;
    try {
        j = json::parse(payload);
    } catch (const json::exception&) {
        sendError("Invalid JSON");
        return;
    }
    // Reject non-object payloads: contains()/operator[] on a string, array
    // or number would throw type_error(305).
    if (!j.is_object()) {
        sendError("Invalid request: expected a JSON object");
        return;
    }
    if (!j.contains(json_key::TYPE)) {
        sendError("Missing 'type' field");
        return;
    }

    j[json_key::CLIENT_ID] = clientId;
    m_incomingQueue.PushWithLimit(j.dump(), MAX_INCOMING_QUEUE_SIZE);
}

// ============================================================================
// Broadcast — deliver a push event only to clients subscribed to its type
// ============================================================================
void WebSocketServer::Broadcast(const std::string& type, const std::string& msg) {
    std::lock_guard<std::mutex> lock(m_clientsMutex);
    for (auto& pair : m_sessions) {
        if (pair.second->handshakeDone && pair.second->subscriptions.count(type) > 0) {
            if (IsSendQueueFull(pair.first, msg.size()))
                continue;  // backpressure: drop frame for a slow consumer
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
    } catch (const json::exception&) {
        return "";
    }
    // find() on a non-object value would throw type_error(305).
    if (!req.is_object())
        return "";

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
        if (IsSendQueueFull(it->second, msg.size())) {
            // Backpressure: drop the request/response for a slow consumer.
            // Log to stderr only (this can repeat while the client is stuck).
            std::cerr << "[DataBridge] Dropping response for slow client " << clientId << std::endl;
            return;
        }
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
            if (IsSendQueueFull(pair.first, msg.size()))
                continue;  // backpressure: drop frame for a slow consumer
            websocketpp::lib::error_code ec;
            m_server.send(pair.first, msg, websocketpp::frame::opcode::text, ec);
        }
    }
}

// ============================================================================
// IsSendQueueFull — backpressure guard against unbounded outbound buffering
// ============================================================================
bool WebSocketServer::IsSendQueueFull(ConnectionHdl hdl, size_t payloadSize) {
    websocketpp::lib::error_code ec;
    auto con = m_server.get_con_from_hdl(hdl, ec);
    if (ec || !con)
        return false;  // dead/unknown connection: let send() report the error
    // Note: MAX_CLIENT_SEND_QUEUE_FRAMES is not enforceable via the public
    // websocketpp API (no frame-count query), so we guard on buffered bytes.
    return con->get_buffered_amount() + payloadSize > MAX_CLIENT_SEND_QUEUE_BYTES;
}

} // namespace edb
