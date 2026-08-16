#include "Plugin.h"
#include "Serializer.h"
#include "Handlers.h"

#include <nlohmann/json.hpp>

#include <iostream>
#include <string>



using json = nlohmann::json;
using namespace edb;
using namespace EuroScopePlugIn;

// ============================================================================
// Construction / Destruction
// ============================================================================

DataBridgePlugin::DataBridgePlugin()
    : EuroScopePlugIn::CPlugIn(EuroScopePlugIn::COMPATIBILITY_CODE,
              PLUGIN_NAME,
              PLUGIN_VERSION,
              PLUGIN_AUTHOR,
              PLUGIN_COPYRIGHT)
{
    m_wsServer = std::make_unique<WebSocketServer>(WS_PORT);

    // Forward WebSocketServer errors to EuroScope's message area
    m_wsServer->SetErrorCallback([this](const std::string& msg) {
        DisplayUserMessage("message", "DataBridge", msg.c_str(), true, false, false, false, false);
    });

    // Register request processor: executed on the server's dedicated drain
    // thread, independent of EuroScope's OnTimer callback. Extracts client_id
    // (injected by OnMessage), routes subscribe/unsubscribe to the
    // subscription manager, otherwise calls HandleRequest and routes the
    // response.
    m_wsServer->SetRequestProcessor([this](std::string&& requestStr) {
        std::string clientId;
        std::string type;
        try {
            json req = json::parse(requestStr);
            // find() on a non-object value would throw type_error(305).
            if (!req.is_object())
                return;
            auto it = req.find(json_key::TYPE);
            if (it != req.end() && it->is_string())
                type = it->get<std::string>();
            auto idIt = req.find(json_key::CLIENT_ID);
            if (idIt != req.end() && idIt->is_string())
                clientId = idIt->get<std::string>();
        } catch (const json::exception&) { return; }

        // Subscription control requests are handled by the WebSocketServer,
        // which owns the per-client subscription state.
        if (type == msg_type::SUBSCRIBE || type == msg_type::UNSUBSCRIBE)
        {
            std::string response = m_wsServer->HandleSubscriptionRequest(requestStr);
            if (!response.empty())
                m_wsServer->RouteOutgoing(std::move(response), clientId);
            return;
        }

        std::string response = HandleRequest(*this, requestStr);
        if (!response.empty())
            m_wsServer->RouteOutgoing(std::move(response), clientId);
    });

    DisplayUserMessage("message", "DataBridgePlugin", std::string("Version " + std::string(PLUGIN_VERSION) + " loaded").c_str(), true, false, false, false, false);

    if (!m_wsServer->Start())
    {
        std::string err = "[DataBridge] WARNING: Plugin loaded but WebSocket server failed to start.";
        std::cerr << err << std::endl;
        DisplayUserMessage("message", "DataBridge", err.c_str(), true, false, false, false, false);
    }
}

DataBridgePlugin::~DataBridgePlugin()
{
    if (m_wsServer)
        m_wsServer->Stop();
}

// ============================================================================
// Helpers
// ============================================================================

void DataBridgePlugin::PushEvent(const char* type, const std::string& dataJson)
{
    try {
        json msg;
        msg[json_key::TYPE] = type;
        msg[json_key::DATA] = json::parse(dataJson.empty() ? "{}" : dataJson);
        m_wsServer->Broadcast(type, msg.dump());
    } catch (...) {
        std::string err = "[DataBridge] Exception in PushEvent(type=";
        err += type; err += ")";
        std::cerr << err << std::endl;
        DisplayUserMessage("message", "DataBridge", err.c_str(), true, false, false, false, false);
    }
}

void DataBridgePlugin::PushEvent(const char* type)
{
    try {
        json msg;
        msg[json_key::TYPE] = type;
        msg[json_key::DATA] = json::object();
        m_wsServer->Broadcast(type, msg.dump());
    } catch (...) {
        std::string err = "[DataBridge] Exception in PushEvent(type=";
        err += type; err += ")";
        std::cerr << err << std::endl;
        DisplayUserMessage("message", "DataBridge", err.c_str(), true, false, false, false, false);
    }
}

void DataBridgePlugin::LogCallbackError(const char* callbackName)
{
    std::string err = "[DataBridge] Exception in ";
    err += callbackName;
    std::cerr << err << std::endl;
    DisplayUserMessage("message", "DataBridge", err.c_str(), true, false, false, false, false);
}

// ============================================================================
// EuroScope Callbacks — Radar / FlightPlan
// ============================================================================

void DataBridgePlugin::OnRadarTargetPositionUpdate(CRadarTarget RadarTarget)
{
    if (!m_wsServer->HasSubscribers(msg_type::RADAR_UPDATE))
        return;
    try {
        if (!RadarTarget.IsValid())
            return;
        std::string data = SerializeRadarTargetToJson(RadarTarget);
        PushEvent(msg_type::RADAR_UPDATE, data);
    } catch (...) {
        LogCallbackError("OnRadarTargetPositionUpdate");
    }
}

void DataBridgePlugin::OnFlightPlanFlightPlanDataUpdate(CFlightPlan FlightPlan)
{
    if (!m_wsServer->HasSubscribers(msg_type::FLIGHTPLAN_UPDATE))
        return;
    try {
        if (!FlightPlan.IsValid())
            return;
        std::string data = SerializeFlightPlanToJson(FlightPlan);
        PushEvent(msg_type::FLIGHTPLAN_UPDATE, data);
    } catch (...) {
        LogCallbackError("OnFlightPlanFlightPlanDataUpdate");
    }
}

void DataBridgePlugin::OnFlightPlanDisconnect(CFlightPlan FlightPlan)
{
    if (!m_wsServer->HasSubscribers(msg_type::FLIGHTPLAN_DISCONNECT))
        return;
    try {
        json data;
        data[json_key::CALLSIGN] = FlightPlan.IsValid() ? FlightPlan.GetCallsign() : "";
        PushEvent(msg_type::FLIGHTPLAN_DISCONNECT, data.dump());
    } catch (...) {
        LogCallbackError("OnFlightPlanDisconnect");
    }
}

void DataBridgePlugin::OnFlightPlanControllerAssignedDataUpdate(CFlightPlan FlightPlan, int DataType)
{
    if (!m_wsServer->HasSubscribers(msg_type::CONTROLLER_ASSIGNED_DATA))
        return;
    try {
        if (!FlightPlan.IsValid())
            return;
        std::string fpJson = SerializeFlightPlanToJson(FlightPlan);
        json msg;
        msg[json_key::TYPE] = msg_type::CONTROLLER_ASSIGNED_DATA;
        msg[json_key::DATA] = json::parse(fpJson);
        msg[json_key::DATA]["data_type"] = DataType;
        m_wsServer->Broadcast(msg_type::CONTROLLER_ASSIGNED_DATA, msg.dump());
    } catch (...) {
        LogCallbackError("OnFlightPlanControllerAssignedDataUpdate");
    }
}

void DataBridgePlugin::OnFlightPlanFlightStripPushed(CFlightPlan FlightPlan,
                                                      const char* sSenderController,
                                                      const char* sTargetController)
{
    if (!m_wsServer->HasSubscribers(msg_type::FLIGHT_STRIP_PUSHED))
        return;
    try {
        json data;
        data[json_key::CALLSIGN] = FlightPlan.IsValid() ? FlightPlan.GetCallsign() : "";
        data["sender"] = sSenderController ? sSenderController : "";
        data["target"] = sTargetController ? sTargetController : "";
        PushEvent(msg_type::FLIGHT_STRIP_PUSHED, data.dump());
    } catch (...) {
        LogCallbackError("OnFlightPlanFlightStripPushed");
    }
}

// ============================================================================
// EuroScope Callbacks — Controller
// ============================================================================

void DataBridgePlugin::OnControllerPositionUpdate(CController Controller)
{
    if (!m_wsServer->HasSubscribers(msg_type::CONTROLLER_UPDATE))
        return;
    try {
        if (!Controller.IsValid())
            return;
        std::string data = SerializeControllerToJson(Controller);
        PushEvent(msg_type::CONTROLLER_UPDATE, data);
    } catch (...) {
        LogCallbackError("OnControllerPositionUpdate");
    }
}

void DataBridgePlugin::OnControllerDisconnect(CController Controller)
{
    if (!m_wsServer->HasSubscribers(msg_type::CONTROLLER_DISCONNECT))
        return;
    try {
        json data;
        if (Controller.IsValid())
        {
            data[json_key::CALLSIGN] = Controller.GetCallsign();
            data["position_id"] = Controller.GetPositionId();
        }
        PushEvent(msg_type::CONTROLLER_DISCONNECT, data.dump());
    } catch (...) {
        LogCallbackError("OnControllerDisconnect");
    }
}

// ============================================================================
// EuroScope Callbacks — Plane Info, Chat, Metar
// ============================================================================

void DataBridgePlugin::OnPlaneInformationUpdate(const char* sCallsign,
                                                 const char* sLivery,
                                                 const char* sPlaneType)
{
    if (!m_wsServer->HasSubscribers(msg_type::PLANE_INFO))
        return;
    try {
        json data;
        data[json_key::CALLSIGN] = sCallsign ? sCallsign : "";
        data["livery"] = sLivery ? sLivery : "";
        data["plane_type"] = sPlaneType ? sPlaneType : "";
        PushEvent(msg_type::PLANE_INFO, data.dump());
    } catch (...) {
        LogCallbackError("OnPlaneInformationUpdate");
    }
}

void DataBridgePlugin::OnCompilePrivateChat(const char* sSenderCallsign,
                                             const char* sReceiverCallsign,
                                             const char* sChatMessage)
{
    if (!m_wsServer->HasSubscribers(msg_type::CHAT_PRIVATE))
        return;
    try {
        json data;
        data["sender"] = sSenderCallsign ? sSenderCallsign : "";
        data["receiver"] = sReceiverCallsign ? sReceiverCallsign : "";
        data["message"] = sChatMessage ? sChatMessage : "";
        PushEvent(msg_type::CHAT_PRIVATE, data.dump());
    } catch (...) {
        LogCallbackError("OnCompilePrivateChat");
    }
}

void DataBridgePlugin::OnCompileFrequencyChat(const char* sSenderCallsign,
                                               double Frequency,
                                               const char* sChatMessage)
{
    if (!m_wsServer->HasSubscribers(msg_type::CHAT_FREQUENCY))
        return;
    try {
        json data;
        data["sender"] = sSenderCallsign ? sSenderCallsign : "";
        data["frequency"] = Frequency;
        data["message"] = sChatMessage ? sChatMessage : "";
        PushEvent(msg_type::CHAT_FREQUENCY, data.dump());
    } catch (...) {
        LogCallbackError("OnCompileFrequencyChat");
    }
}

void DataBridgePlugin::OnNewMetarReceived(const char* sStation, const char* sFullMetar)
{
    if (!m_wsServer->HasSubscribers(msg_type::METAR_RECEIVED))
        return;
    try {
        json data;
        data["station"] = sStation ? sStation : "";
        data["metar"] = sFullMetar ? sFullMetar : "";
        PushEvent(msg_type::METAR_RECEIVED, data.dump());
    } catch (...) {
        LogCallbackError("OnNewMetarReceived");
    }
}

bool DataBridgePlugin::OnCompileCommand(const char* sCommandLine)
{
    // Not intercepted by default — clients use "send_command" request type.
    return false;
}

void DataBridgePlugin::OnAirportRunwayActivityChanged(void)
{
    if (!m_wsServer->HasSubscribers(msg_type::AIRPORT_RUNWAY_ACTIVITY_CHANGED))
        return;
    try {
        PushEvent(msg_type::AIRPORT_RUNWAY_ACTIVITY_CHANGED);
    } catch (...) {
        LogCallbackError("OnAirportRunwayActivityChanged");
    }
}

// ============================================================================
// OnTimer — Send periodic events
// ============================================================================

void DataBridgePlugin::OnTimer(int Counter)
{
    try {
    // Incoming WebSocket requests are processed inline by the server's
    // message handler (ASIO IO thread) and responded to immediately — not
    // here, and not via an interval-triggered queue.

    // Send timer event every second (only to subscribers)
    if (m_wsServer->HasSubscribers(msg_type::TIMER))
    {
        json timerMsg;
        timerMsg[json_key::TYPE] = msg_type::TIMER;
        timerMsg[json_key::DATA][json_key::COUNTER] = Counter;
        m_wsServer->Broadcast(msg_type::TIMER, timerMsg.dump());
    }
    } catch (...) {
        std::string err = "[DataBridge] Exception in OnTimer";
        std::cerr << err << std::endl;
        DisplayUserMessage("message", "DataBridge", err.c_str(), true, false, false, false, false);
    }
}


