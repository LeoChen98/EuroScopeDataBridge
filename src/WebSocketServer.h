#pragma once

#include <string>
#include <memory>
#include <atomic>
#include <thread>
#include <vector>
#include <mutex>
#include <ixwebsocket/IXWebSocketServer.h>

#include "ThreadSafeQueue.h"

namespace edb {

// ============================================================================
// WebSocketServer — wraps IXWebSocketServer for local EuroScope data bridge
// ============================================================================
class WebSocketServer {
public:
    // Construct with references to the two queues shared with the plugin.
    // incomingQueue: messages from clients → plugin (requests)
    // outgoingQueue: messages from plugin → clients (push broadcasts)
    WebSocketServer(int port, ThreadSafeQueue& incomingQueue, ThreadSafeQueue& outgoingQueue);
    ~WebSocketServer();

    // Non-copyable, non-movable
    WebSocketServer(const WebSocketServer&) = delete;
    WebSocketServer& operator=(const WebSocketServer&) = delete;

    // Start the server. Returns true on success.
    bool Start();

    // Stop the server and join the broadcast thread.
    void Stop();

    // Whether the server is running.
    bool IsRunning() const { return m_running.load(); }

private:
    int m_port;
    ThreadSafeQueue& m_incomingQueue;
    ThreadSafeQueue& m_outgoingQueue;

    ix::WebSocketServer m_server;
    std::thread m_broadcastThread;
    std::atomic<bool> m_running{false};

    // Thread-safe list of connected clients
    std::mutex m_clientsMutex;
    std::vector<std::shared_ptr<ix::WebSocket>> m_clients;
};

} // namespace edb
