#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif

// Required for WebSocket++ to use standalone Asio (not Boost.Asio)
#ifndef ASIO_STANDALONE
#define ASIO_STANDALONE
#endif

// WebSocket++ C++11 feature detection: MSVC needs explicit opt-in
// because /Zc:__cplusplus is not set and __cplusplus reports C++98.
#ifndef _WEBSOCKETPP_CPP11_STL_
#define _WEBSOCKETPP_CPP11_STL_
#endif

#include <string>
#include <memory>
#include <atomic>
#include <thread>
#include <map>
#include <set>
#include <mutex>
#include <functional>
#include <windows.h>

#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>

#include "ThreadSafeQueue.h"

namespace edb {

// ============================================================================
// WebSocketServer — WebSocket++ based server for local EuroScope
//
// Architecture:
//   1 ASIO io_service thread (async I/O for all connections)
//   + OnTimer-driven request processing on the EuroScope main thread
//
// Messages are routed per-client:
//   - push events broadcast to all handshake-complete clients
//   - request responses delivered only to the originating client
// ============================================================================
class WebSocketServer {
public:
    // WsServer type alias (no TLS — localhost only)
    using WsServer = websocketpp::server<websocketpp::config::asio>;
    using ConnectionHdl = websocketpp::connection_hdl;

    // Constructor: port + shared incoming queue (EuroScope thread drains it).
    WebSocketServer(int port, ThreadSafeQueue& incomingQueue);
    ~WebSocketServer();

    // Non-copyable, non-movable
    WebSocketServer(const WebSocketServer&) = delete;
    WebSocketServer& operator=(const WebSocketServer&) = delete;

    // Start the server. Returns true on success.
    bool Start();

    // Stop the server and join all threads.
    void Stop();

    // Whether the server is running.
    bool IsRunning() const { return m_running.load(); }

    // --- Message routing ---
    //
    // Broadcast: deliver a push event to every client that subscribed to the
    // given event type (subscription-based push, not default broadcast).
    // Used for push events (radar_update, timer, full_snapshot, etc.).
    void Broadcast(const std::string& type, const std::string& msg);

    // HasSubscribers: whether at least one connected client has subscribed to
    // the given event type. Lets EuroScope callbacks skip their work entirely
    // when nobody is interested in the event.
    bool HasSubscribers(const std::string& type);

    // HandleSubscriptionRequest: process a "subscribe" / "unsubscribe" request
    // (client_id is injected into the request JSON by OnMessage). Returns the
    // JSON response to send back to the client, or an empty string if the
    // request should be ignored.
    std::string HandleSubscriptionRequest(const std::string& requestJson);

    // RouteOutgoing: deliver a message to a specific client (by clientId),
    // or broadcast to all when clientId is empty.
    void RouteOutgoing(const std::string& msg, const std::string& clientId);

    // Generate a UUID string for client identification.
    static std::string GenerateClientId();

    // Set a callback for error/info messages (e.g., EuroScope DisplayUserMessage).
    // Pass nullptr to disable the callback.
    void SetErrorCallback(std::function<void(const std::string&)> cb) {
        m_errorCallback = std::move(cb);
    }

    // Set a callback for processing incoming requests.
    // Called from the EuroScope main thread (via DrainIncomingQueue / OnTimer).
    // The callback receives the raw JSON request string (with client_id already injected).
    void SetRequestProcessor(std::function<void(std::string&&)> processor) {
        m_requestProcessor = std::move(processor);
    }

    // Drain and process all pending incoming messages.
    // Call from the EuroScope main thread (OnTimer) periodically.
    void DrainIncomingQueue();

private:
    // --- Per-client session ---
    struct ClientSession {
        std::string clientId;       // UUID, assigned in on_open
        bool handshakeDone = false;
        std::set<std::string> subscriptions;  // subscribed push event types

        explicit ClientSession(std::string id)
            : clientId(std::move(id)), handshakeDone(true) {}
    };

    // Send a message to every handshake-complete client (no subscription filter).
    // Caller must hold m_clientsMutex.
    void SendToAllLocked(const std::string& msg);

    // Backpressure check: returns true if queueing payloadSize more bytes on
    // this client would exceed MAX_CLIENT_SEND_QUEUE_BYTES (slow consumer guard,
    // prevents unbounded memory growth / bad_alloc). Caller must hold m_clientsMutex.
    bool IsSendQueueFull(ConnectionHdl hdl, size_t payloadSize);

    // --- WebSocket++ event handlers ---
    bool OnValidate(ConnectionHdl hdl);
    void OnOpen(ConnectionHdl hdl);
    void OnClose(ConnectionHdl hdl);
    void OnMessage(ConnectionHdl hdl, WsServer::message_ptr msg);
    void OnFail(ConnectionHdl hdl);

    // Log an error to both stderr and the optional callback
    void LogError(const std::string& msg) {
        std::cerr << msg << std::endl;
        if (m_errorCallback) m_errorCallback(msg);
    }

    int m_port;
    ThreadSafeQueue& m_incomingQueue;

    WsServer m_server;
    std::thread m_ioThread;
    std::thread m_drainThread;
    std::atomic<bool> m_running;
    std::atomic<bool> m_drainRunning{false};

    // Client session management (protected by m_clientsMutex)
    std::mutex m_clientsMutex;
    std::map<ConnectionHdl, std::shared_ptr<ClientSession>, std::owner_less<ConnectionHdl>> m_sessions;
    std::map<std::string, ConnectionHdl> m_clientIdToHdl;
    // Per-event-type subscriber count (protected by m_clientsMutex) so that
    // HasSubscribers is an O(1) lookup on the hot callback path.
    std::map<std::string, size_t> m_subscriptionCounts;

    // Callbacks
    std::function<void(const std::string&)> m_errorCallback;
    std::function<void(std::string&&)> m_requestProcessor;
};

} // namespace edb
