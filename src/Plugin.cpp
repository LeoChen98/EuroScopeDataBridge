#include "Plugin.h"
#include "Serializer.h"
#include "Handlers.h"

#include <nlohmann/json.hpp>

#include <iostream>

using json = nlohmann::json;
using namespace edb;
using namespace EuroScopePlugIn;

// ============================================================================
// Construction / Destruction
// ============================================================================

DataBridgePlugin::DataBridgePlugin()
    : CPlugIn(edb::COMPATIBILITY_CODE,
              PLUGIN_NAME,
              PLUGIN_VERSION,
              PLUGIN_AUTHOR,
              PLUGIN_COPYRIGHT)
{
    m_wsServer = std::make_unique<WebSocketServer>(WS_PORT, m_incomingQueue, m_outgoingQueue);

    if (!m_wsServer->Start())
    {
        std::cerr << "[DataBridge] WARNING: Plugin loaded but WebSocket server failed to start."
                  << std::endl;
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
    json msg;
    msg[json_key::TYPE] = type;
    msg[json_key::DATA] = json::parse(dataJson.empty() ? "{}" : dataJson);
    m_outgoingQueue.Push(msg.dump());
}

void DataBridgePlugin::PushEvent(const char* type)
{
    json msg;
    msg[json_key::TYPE] = type;
    msg[json_key::DATA] = json::object();
    m_outgoingQueue.Push(msg.dump());
}

// ============================================================================
// EuroScope Callbacks — Radar / FlightPlan
// ============================================================================

void DataBridgePlugin::OnRadarTargetPositionUpdate(CRadarTarget RadarTarget)
{
    if (!RadarTarget.IsValid())
        return;
    std::string data = SerializeRadarTargetToJson(RadarTarget);
    PushEvent(msg_type::RADAR_UPDATE, data);
}

void DataBridgePlugin::OnFlightPlanFlightPlanDataUpdate(CFlightPlan FlightPlan)
{
    if (!FlightPlan.IsValid())
        return;
    std::string data = SerializeFlightPlanToJson(FlightPlan);
    PushEvent(msg_type::FLIGHTPLAN_UPDATE, data);
}

void DataBridgePlugin::OnFlightPlanDisconnect(CFlightPlan FlightPlan)
{
    json data;
    data[json_key::CALLSIGN] = FlightPlan.GetCallsign();
    PushEvent(msg_type::FLIGHTPLAN_DISCONNECT, data.dump());
}

void DataBridgePlugin::OnFlightPlanControllerAssignedDataUpdate(CFlightPlan FlightPlan, int DataType)
{
    if (!FlightPlan.IsValid())
        return;
    std::string fpJson = SerializeFlightPlanToJson(FlightPlan);
    json msg;
    msg[json_key::TYPE] = msg_type::CONTROLLER_ASSIGNED_DATA;
    msg[json_key::DATA] = json::parse(fpJson);
    msg[json_key::DATA]["data_type"] = DataType;
    m_outgoingQueue.Push(msg.dump());
}

void DataBridgePlugin::OnFlightPlanFlightStripPushed(CFlightPlan FlightPlan,
                                                      const char* sSenderController,
                                                      const char* sTargetController)
{
    json data;
    data[json_key::CALLSIGN] = FlightPlan.IsValid() ? FlightPlan.GetCallsign() : "";
    data["sender"] = sSenderController ? sSenderController : "";
    data["target"] = sTargetController ? sTargetController : "";
    PushEvent(msg_type::FLIGHT_STRIP_PUSHED, data.dump());
}

// ============================================================================
// EuroScope Callbacks — Controller
// ============================================================================

void DataBridgePlugin::OnControllerPositionUpdate(CController Controller)
{
    if (!Controller.IsValid())
        return;
    std::string data = SerializeControllerToJson(Controller);
    PushEvent(msg_type::CONTROLLER_UPDATE, data);
}

void DataBridgePlugin::OnControllerDisconnect(CController Controller)
{
    json data;
    if (Controller.IsValid())
    {
        data[json_key::CALLSIGN] = Controller.GetCallsign();
        data["position_id"] = Controller.GetPositionId();
    }
    PushEvent(msg_type::CONTROLLER_DISCONNECT, data.dump());
}

// ============================================================================
// EuroScope Callbacks — Plane Info, Chat, Metar
// ============================================================================

void DataBridgePlugin::OnPlaneInformationUpdate(const char* sCallsign,
                                                 const char* sLivery,
                                                 const char* sPlaneType)
{
    json data;
    data[json_key::CALLSIGN] = sCallsign ? sCallsign : "";
    data["livery"] = sLivery ? sLivery : "";
    data["plane_type"] = sPlaneType ? sPlaneType : "";
    PushEvent(msg_type::PLANE_INFO, data.dump());
}

void DataBridgePlugin::OnCompilePrivateChat(const char* sSenderCallsign,
                                             const char* sReceiverCallsign,
                                             const char* sChatMessage)
{
    json data;
    data["sender"] = sSenderCallsign ? sSenderCallsign : "";
    data["receiver"] = sReceiverCallsign ? sReceiverCallsign : "";
    data["message"] = sChatMessage ? sChatMessage : "";
    PushEvent(msg_type::CHAT_PRIVATE, data.dump());
}

void DataBridgePlugin::OnCompileFrequencyChat(const char* sSenderCallsign,
                                               double Frequency,
                                               const char* sChatMessage)
{
    json data;
    data["sender"] = sSenderCallsign ? sSenderCallsign : "";
    data["frequency"] = Frequency;
    data["message"] = sChatMessage ? sChatMessage : "";
    PushEvent(msg_type::CHAT_FREQUENCY, data.dump());
}

void DataBridgePlugin::OnNewMetarReceived(const char* sStation, const char* sFullMetar)
{
    json data;
    data["station"] = sStation ? sStation : "";
    data["metar"] = sFullMetar ? sFullMetar : "";
    PushEvent(msg_type::METAR_RECEIVED, data.dump());
}

bool DataBridgePlugin::OnCompileCommand(const char* sCommandLine)
{
    // Not intercepted by default — clients use "send_command" request type.
    return false;
}

void DataBridgePlugin::OnAirportRunwayActivityChanged(void)
{
    PushEvent("airport_runway_activity_changed");
}

// ============================================================================
// OnTimer — Process incoming requests + send periodic events
// ============================================================================

void DataBridgePlugin::OnTimer(int Counter)
{
    // 1. Process incoming WebSocket requests (on ES thread for thread safety)
    m_incomingQueue.Drain([this](std::string&& requestStr) {
        std::string response = HandleRequest(*this, requestStr);
        if (!response.empty())
        {
            m_outgoingQueue.Push(std::move(response));
        }
    });

    // 2. Send periodic full snapshot
    if (Counter % FULL_SNAPSHOT_INTERVAL == 0 && Counter > 0)
    {
        SendFullSnapshot();
    }

    // 3. Send timer event every second
    json timerMsg;
    timerMsg[json_key::TYPE] = msg_type::TIMER;
    timerMsg[json_key::DATA][json_key::COUNTER] = Counter;
    m_outgoingQueue.Push(timerMsg.dump());
}

// ============================================================================
// SendFullSnapshot — periodic complete state dump
// ============================================================================

void DataBridgePlugin::SendFullSnapshot()
{
    json snapshot;
    snapshot[json_key::TYPE] = msg_type::FULL_SNAPSHOT;

    json& data = snapshot[json_key::DATA];
    data["flightplans"] = json::array();
    data["radar_targets"] = json::array();
    data["controllers"] = json::array();

    // Iterate all flight plans
    CFlightPlan fp = FlightPlanSelectFirst();
    while (fp.IsValid())
    {
        std::string fpJson = SerializeFlightPlanToJson(fp);
        data["flightplans"].push_back(json::parse(fpJson));
        fp = FlightPlanSelectNext(fp);
    }

    // Iterate all radar targets
    CRadarTarget rt = RadarTargetSelectFirst();
    while (rt.IsValid())
    {
        std::string rtJson = SerializeRadarTargetToJson(rt);
        data["radar_targets"].push_back(json::parse(rtJson));
        rt = RadarTargetSelectNext(rt);
    }

    // Iterate all controllers
    CController ctr = ControllerSelectFirst();
    while (ctr.IsValid())
    {
        std::string ctrJson = SerializeControllerToJson(ctr);
        data["controllers"].push_back(json::parse(ctrJson));
        ctr = ControllerSelectNext(ctr);
    }

    // Include self
    CController myself = ControllerMyself();
    if (myself.IsValid())
    {
        std::string myJson = SerializeControllerToJson(myself);
        data["controllers"].push_back(json::parse(myJson));
    }

    m_outgoingQueue.Push(snapshot.dump());
}
