#include "WebSocketServer.h"
#include "Constants.h"

#include <ixwebsocket/IXNetSystem.h>
#include <ixwebsocket/IXWebSocketServer.h>

#include <nlohmann/json.hpp>

#include <chrono>
#include <iostream>

using json = nlohmann::json;

namespace edb {

WebSocketServer::WebSocketServer(int port, ThreadSafeQueue& incomingQueue, ThreadSafeQueue& outgoingQueue)
    : m_port(port)
    , m_incomingQueue(incomingQueue)
    , m_outgoingQueue(outgoingQueue)
    , m_server(port, "127.0.0.1")
{
    ix::initNetSystem();
}

WebSocketServer::~WebSocketServer()
{
    Stop();
    ix::uninitNetSystem();
}

bool WebSocketServer::Start()
{
    if (m_running.load())
        return false;

    // Set up connection callback (ixwebsocket v11.4.6 uses weak_ptr)
    m_server.setOnConnectionCallback(
        [this](std::weak_ptr<ix::WebSocket> weakWS,
               std::shared_ptr<ix::ConnectionState> /*connectionState*/)
        {
            auto ws = weakWS.lock();
            if (!ws) return;

            {
                std::lock_guard<std::mutex> lock(m_clientsMutex);
                m_clients.push_back(ws);
            }

            ws->setOnMessageCallback(
                [this, ws](const ix::WebSocketMessagePtr& msg)
                {
                    if (msg->type == ix::WebSocketMessageType::Message)
                    {
                        // Forward client message to incoming queue
                        try
                        {
                            json j = json::parse(msg->str);
                            // Ensure the message has at least a "type" field
                            if (j.contains(json_key::TYPE))
                            {
                                m_incomingQueue.Push(msg->str);
                            }
                        }
                        catch (const json::parse_error&)
                        {
                            // Malformed JSON — send error back
                            json err;
                            err[json_key::TYPE] = msg_type::ERROR;
                            err[json_key::DATA][json_key::ERROR] = "Invalid JSON";
                            ws->sendText(err.dump());
                        }
                    }
                }
            );
        }
    );

    if (!m_server.listenAndStart())
    {
        std::cerr << "[DataBridge] Failed to start WebSocket server on port "
                  << m_port << std::endl;
        return false;
    }

    m_running.store(true);

    // Start broadcast thread
    m_broadcastThread = std::thread([this]() {
        while (m_running.load())
        {
            // Wait briefly
            std::this_thread::sleep_for(std::chrono::milliseconds(WS_OUTGOING_INTERVAL_MS));

            // Drain outgoing queue → broadcast to all clients
            std::lock_guard<std::mutex> lock(m_clientsMutex);

            // Remove disconnected clients
            m_clients.erase(
                std::remove_if(m_clients.begin(), m_clients.end(),
                    [](const std::shared_ptr<ix::WebSocket>& ws) {
                        return ws->getReadyState() == ix::ReadyState::Closed;
                    }),
                m_clients.end()
            );

            // Broadcast all pending messages
            m_outgoingQueue.Drain([this](std::string&& msg) {
                for (auto& ws : m_clients)
                {
                    if (ws->getReadyState() == ix::ReadyState::Open)
                    {
                        ws->sendText(msg);
                    }
                }
            });
        }
    });

    std::cout << "[DataBridge] WebSocket server listening on ws://127.0.0.1:" << m_port << std::endl;
    return true;
}

void WebSocketServer::Stop()
{
    m_running.store(false);
    if (m_broadcastThread.joinable())
    {
        m_broadcastThread.join();
    }
    m_server.stop();
}

} // namespace edb
