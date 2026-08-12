#include "Handlers.h"
#include "Serializer.h"
#include "Constants.h"

#include <nlohmann/json.hpp>

#include <string>
#include <vector>

using json = nlohmann::json;
using namespace EuroScopePlugIn;
using namespace edb;

namespace {

// ============================================================================
// Response helpers
// ============================================================================

std::string MakeResponse(const std::string& id, bool success, const std::string& resultJson, const std::string& error)
{
    json resp;
    resp[json_key::TYPE] = msg_type::RESPONSE;
    resp[json_key::ID] = id;
    resp[json_key::DATA][json_key::SUCCESS] = success;
    if (!resultJson.empty())
        resp[json_key::DATA]["result"] = json::parse(resultJson);
    if (!error.empty())
        resp[json_key::DATA][json_key::ERROR] = error;
    return resp.dump();
}

std::string MakeSuccess(const std::string& id, const std::string& resultJson = "")
{
    return MakeResponse(id, true, resultJson, "");
}

std::string MakeError(const std::string& id, const std::string& error)
{
    return MakeResponse(id, false, "", error);
}

// Get a JSON string value from data, return empty string if not present
std::string JStr(const json& data, const char* key, const std::string& defaultVal = "")
{
    auto it = data.find(key);
    if (it != data.end() && it->is_string())
        return it->get<std::string>();
    return defaultVal;
}

// Get a JSON int value from data, return default if not present
int JInt(const json& data, const char* key, int defaultVal = 0)
{
    auto it = data.find(key);
    if (it != data.end() && it->is_number_integer())
        return it->get<int>();
    return defaultVal;
}

// Get a JSON bool value from data, return default if not present
bool JBool(const json& data, const char* key, bool defaultVal = false)
{
    auto it = data.find(key);
    if (it != data.end() && it->is_boolean())
        return it->get<bool>();
    return defaultVal;
}

} // anonymous namespace

namespace edb {

// ============================================================================
// HandleRequest - main dispatch
// ============================================================================

std::string HandleRequest(CPlugIn& plugin, const std::string& requestJson)
{
    json req;
    try {
        req = json::parse(requestJson);
    } catch (const json::parse_error&) {
        // Malformed JSON; ignore silently
        return "";
    }

    std::string id;
    auto idIt = req.find(json_key::ID);
    if (idIt != req.end() && idIt->is_string())
        id = idIt->get<std::string>();

    auto typeIt = req.find(json_key::TYPE);
    if (typeIt == req.end() || !typeIt->is_string())
        return MakeError(id, "Missing 'type' field");

    std::string type = typeIt->get<std::string>();
    const json& data = req.contains(json_key::DATA) && req[json_key::DATA].is_object()
                       ? req[json_key::DATA] : json::object();

    const std::string callsign = JStr(data, json_key::CALLSIGN);
    const std::string value    = JStr(data, json_key::VALUE);
    const int intValue         = JInt(data, json_key::VALUE);

    // ========================================================================
    // Query Handlers (get_*)
    // ========================================================================

    if (type == msg_type::GET_FLIGHTPLANS)
    {
        json result = json::array();
        if (!callsign.empty())
        {
            CFlightPlan fp = plugin.FlightPlanSelect(callsign.c_str());
            if (fp.IsValid())
                result.push_back(json::parse(SerializeFlightPlanToJson(fp)));
        }
        else
        {
            CFlightPlan fp = plugin.FlightPlanSelectFirst();
            while (fp.IsValid())
            {
                try {
                    result.push_back(json::parse(SerializeFlightPlanToJson(fp)));
                }
                catch (...) {
                }

                fp = plugin.FlightPlanSelectNext(fp);
            }
        }
        return MakeSuccess(id, result.dump());
    }

    if (type == msg_type::GET_RADAR_TARGETS)
    {
        json result = json::array();
        if (!callsign.empty())
        {
            CRadarTarget rt = plugin.RadarTargetSelect(callsign.c_str());
            if (rt.IsValid())
                result.push_back(json::parse(SerializeRadarTargetToJson(rt)));
        }
        else
        {
            CRadarTarget rt = plugin.RadarTargetSelectFirst();
            while (rt.IsValid())
            {
                result.push_back(json::parse(SerializeRadarTargetToJson(rt)));
                rt = plugin.RadarTargetSelectNext(rt);
            }
        }
        return MakeSuccess(id, result.dump());
    }

    if (type == msg_type::GET_CONTROLLERS)
    {
        json result = json::array();

        CController myself = plugin.ControllerMyself();
        if (myself.IsValid())
            result.push_back(json::parse(SerializeControllerToJson(myself)));

        CController ctr = plugin.ControllerSelectFirst();
        while (ctr.IsValid())
        {
            result.push_back(json::parse(SerializeControllerToJson(ctr)));
            ctr = plugin.ControllerSelectNext(ctr);
        }
        return MakeSuccess(id, result.dump());
    }

    if (type == msg_type::GET_SECTOR_ELEMENTS)
    {
        int filterType = JInt(data, "element_type", -1);  // -1 = ALL
        json result = json::array();

        plugin.SelectActiveSectorfile();
        CSectorElement se = plugin.SectorFileElementSelectFirst(filterType);
        while (se.IsValid())
        {
            result.push_back(json::parse(SerializeSectorElementToJson(se)));
            se = plugin.SectorFileElementSelectNext(se, filterType);
        }
        return MakeSuccess(id, result.dump());
    }

    if (type == msg_type::GET_VOICE_CHANNELS)
    {
        json result = json::array();
        CGrountToAirChannel ch = plugin.GroundToArChannelSelectFirst();
        while (ch.IsValid())
        {
            result.push_back(json::parse(SerializeVoiceChannelToJson(ch)));
            ch = plugin.GroundToArChannelSelectNext(ch);
        }
        return MakeSuccess(id, result.dump());
    }

    if (type == msg_type::GET_TRANSITION_ALTITUDE)
    {
        int ta = plugin.GetTransitionAltitude();
        json result;
        result["transition_altitude"] = ta;
        return MakeSuccess(id, result.dump());
    }

    if (type == msg_type::GET_CONNECTION_TYPE)
    {
        int ct = plugin.GetConnectionType();
        json result;
        result["connection_type"] = ct;
        return MakeSuccess(id, result.dump());
    }

    if (type == msg_type::GET_FULL_SNAPSHOT)
    {
        json result;
        result["flightplans"] = json::array();
        result["radar_targets"] = json::array();
        result["controllers"] = json::array();

        // Iterate all flight plans
        CFlightPlan fp = plugin.FlightPlanSelectFirst();
        while (fp.IsValid())
        {
            std::string fpJson = SerializeFlightPlanToJson(fp);
            result["flightplans"].push_back(json::parse(fpJson));
            fp = plugin.FlightPlanSelectNext(fp);
        }

        // Iterate all radar targets
        CRadarTarget rt = plugin.RadarTargetSelectFirst();
        while (rt.IsValid())
        {
            std::string rtJson = SerializeRadarTargetToJson(rt);
            result["radar_targets"].push_back(json::parse(rtJson));
            rt = plugin.RadarTargetSelectNext(rt);
        }

        // Iterate all controllers
        CController ctr = plugin.ControllerSelectFirst();
        while (ctr.IsValid())
        {
            std::string ctrJson = SerializeControllerToJson(ctr);
            result["controllers"].push_back(json::parse(ctrJson));
            ctr = plugin.ControllerSelectNext(ctr);
        }

        // Include self
        CController myself = plugin.ControllerMyself();
        if (myself.IsValid())
        {
            std::string myJson = SerializeControllerToJson(myself);
            result["controllers"].push_back(json::parse(myJson));
        }

        return MakeSuccess(id, result.dump());
    }

    // ========================================================================
    // Setter Handlers - ControllerAssignedData (via FP)
    // ========================================================================

    if (callsign.empty() && type.rfind("set_", 0) == 0 && type != msg_type::SET_ASEL)
        return MakeError(id, "Missing 'callsign' for setter operation");

    auto lookupFp = [&](const std::string& cs) -> CFlightPlan {
        CFlightPlan fp = plugin.FlightPlanSelect(cs.c_str());
        return fp;
    };

    if (type == msg_type::SET_SQUAWK)
    {
        CFlightPlan fp = lookupFp(callsign);
        if (!fp.IsValid()) return MakeError(id, "Flight plan not found: " + callsign);
        CFlightPlanControllerAssignedData cad = fp.GetControllerAssignedData();
        bool ok = cad.SetSquawk(value.c_str());
        return MakeSuccess(id, ok ? "{}" : "");
    }

    if (type == msg_type::SET_FINAL_ALTITUDE)
    {
        // Note: this sets the controller-override final altitude on the assigned data,
        // not the flight plan data field
        CFlightPlan fp = lookupFp(callsign);
        if (!fp.IsValid()) return MakeError(id, "Flight plan not found: " + callsign);
        CFlightPlanControllerAssignedData cad = fp.GetControllerAssignedData();
        bool ok = cad.SetFinalAltitude(intValue);
        return MakeSuccess(id, ok ? "{}" : "");
    }

    if (type == msg_type::SET_CLEARED_ALTITUDE)
    {
        CFlightPlan fp = lookupFp(callsign);
        if (!fp.IsValid()) return MakeError(id, "Flight plan not found: " + callsign);
        CFlightPlanControllerAssignedData cad = fp.GetControllerAssignedData();
        bool ok = cad.SetClearedAltitude(intValue);
        return MakeSuccess(id, ok ? "{}" : "");
    }

    if (type == msg_type::SET_SCRATCHPAD)
    {
        CFlightPlan fp = lookupFp(callsign);
        if (!fp.IsValid()) return MakeError(id, "Flight plan not found: " + callsign);
        CFlightPlanControllerAssignedData cad = fp.GetControllerAssignedData();
        bool ok = cad.SetScratchPadString(value.c_str());
        return MakeSuccess(id, ok ? "{}" : "");
    }

    if (type == msg_type::SET_SPEED)
    {
        CFlightPlan fp = lookupFp(callsign);
        if (!fp.IsValid()) return MakeError(id, "Flight plan not found: " + callsign);
        CFlightPlanControllerAssignedData cad = fp.GetControllerAssignedData();
        bool ok = cad.SetAssignedSpeed(intValue);
        return MakeSuccess(id, ok ? "{}" : "");
    }

    if (type == msg_type::SET_MACH)
    {
        CFlightPlan fp = lookupFp(callsign);
        if (!fp.IsValid()) return MakeError(id, "Flight plan not found: " + callsign);
        CFlightPlanControllerAssignedData cad = fp.GetControllerAssignedData();
        bool ok = cad.SetAssignedMach(intValue);
        return MakeSuccess(id, ok ? "{}" : "");
    }

    if (type == msg_type::SET_RATE)
    {
        CFlightPlan fp = lookupFp(callsign);
        if (!fp.IsValid()) return MakeError(id, "Flight plan not found: " + callsign);
        CFlightPlanControllerAssignedData cad = fp.GetControllerAssignedData();
        bool ok = cad.SetAssignedRate(intValue);
        return MakeSuccess(id, ok ? "{}" : "");
    }

    if (type == msg_type::SET_HEADING)
    {
        CFlightPlan fp = lookupFp(callsign);
        if (!fp.IsValid()) return MakeError(id, "Flight plan not found: " + callsign);
        CFlightPlanControllerAssignedData cad = fp.GetControllerAssignedData();
        bool ok = cad.SetAssignedHeading(intValue);
        return MakeSuccess(id, ok ? "{}" : "");
    }

    if (type == msg_type::SET_DIRECT_TO)
    {
        CFlightPlan fp = lookupFp(callsign);
        if (!fp.IsValid()) return MakeError(id, "Flight plan not found: " + callsign);
        CFlightPlanControllerAssignedData cad = fp.GetControllerAssignedData();
        bool ok = cad.SetDirectToPointName(value.c_str());
        return MakeSuccess(id, ok ? "{}" : "");
    }

    if (type == msg_type::SET_COMMUNICATION_TYPE)
    {
        CFlightPlan fp = lookupFp(callsign);
        if (!fp.IsValid()) return MakeError(id, "Flight plan not found: " + callsign);
        CFlightPlanControllerAssignedData cad = fp.GetControllerAssignedData();
        bool ok = cad.SetCommunicationType(value.empty() ? 0 : value[0]);
        return MakeSuccess(id, ok ? "{}" : "");
    }

    // ========================================================================
    // Setter Handlers - FlightPlanData setters
    // ========================================================================

    if (type == msg_type::SET_PLAN_TYPE)
    {
        CFlightPlan fp = lookupFp(callsign);
        if (!fp.IsValid()) return MakeError(id, "Flight plan not found: " + callsign);
        CFlightPlanData fpd = fp.GetFlightPlanData();
        bool ok = fpd.SetPlanType(value.c_str());
        return MakeSuccess(id, ok ? "{}" : "");
    }

    if (type == msg_type::SET_AIRCRAFT_INFO)
    {
        CFlightPlan fp = lookupFp(callsign);
        if (!fp.IsValid()) return MakeError(id, "Flight plan not found: " + callsign);
        CFlightPlanData fpd = fp.GetFlightPlanData();
        bool ok = fpd.SetAircraftInfo(value.c_str());
        return MakeSuccess(id, ok ? "{}" : "");
    }

    if (type == msg_type::SET_ORIGIN)
    {
        CFlightPlan fp = lookupFp(callsign);
        if (!fp.IsValid()) return MakeError(id, "Flight plan not found: " + callsign);
        CFlightPlanData fpd = fp.GetFlightPlanData();
        bool ok = fpd.SetOrigin(value.c_str());
        return MakeSuccess(id, ok ? "{}" : "");
    }

    if (type == msg_type::SET_DESTINATION)
    {
        CFlightPlan fp = lookupFp(callsign);
        if (!fp.IsValid()) return MakeError(id, "Flight plan not found: " + callsign);
        CFlightPlanData fpd = fp.GetFlightPlanData();
        bool ok = fpd.SetDestination(value.c_str());
        return MakeSuccess(id, ok ? "{}" : "");
    }

    if (type == msg_type::SET_ALTERNATE)
    {
        CFlightPlan fp = lookupFp(callsign);
        if (!fp.IsValid()) return MakeError(id, "Flight plan not found: " + callsign);
        CFlightPlanData fpd = fp.GetFlightPlanData();
        bool ok = fpd.SetAlternate(value.c_str());
        return MakeSuccess(id, ok ? "{}" : "");
    }

    if (type == msg_type::SET_REMARKS)
    {
        CFlightPlan fp = lookupFp(callsign);
        if (!fp.IsValid()) return MakeError(id, "Flight plan not found: " + callsign);
        CFlightPlanData fpd = fp.GetFlightPlanData();
        bool ok = fpd.SetRemarks(value.c_str());
        return MakeSuccess(id, ok ? "{}" : "");
    }

    if (type == msg_type::SET_ROUTE)
    {
        CFlightPlan fp = lookupFp(callsign);
        if (!fp.IsValid()) return MakeError(id, "Flight plan not found: " + callsign);
        CFlightPlanData fpd = fp.GetFlightPlanData();
        bool ok = fpd.SetRoute(value.c_str());
        return MakeSuccess(id, ok ? "{}" : "");
    }

    if (type == msg_type::SET_TRUE_AIRSPEED)
    {
        CFlightPlan fp = lookupFp(callsign);
        if (!fp.IsValid()) return MakeError(id, "Flight plan not found: " + callsign);
        CFlightPlanData fpd = fp.GetFlightPlanData();
        bool ok = fpd.SetTrueAirspeed(intValue);
        return MakeSuccess(id, ok ? "{}" : "");
    }

    if (type == msg_type::SET_DEPARTURE_TIME)
    {
        CFlightPlan fp = lookupFp(callsign);
        if (!fp.IsValid()) return MakeError(id, "Flight plan not found: " + callsign);
        CFlightPlanData fpd = fp.GetFlightPlanData();
        bool ok = fpd.SetEstimatedDepartureTime(value.c_str());
        return MakeSuccess(id, ok ? "{}" : "");
    }

    if (type == msg_type::SET_ACTUAL_DEPARTURE_TIME)
    {
        CFlightPlan fp = lookupFp(callsign);
        if (!fp.IsValid()) return MakeError(id, "Flight plan not found: " + callsign);
        CFlightPlanData fpd = fp.GetFlightPlanData();
        bool ok = fpd.SetActualDepartureTime(value.c_str());
        return MakeSuccess(id, ok ? "{}" : "");
    }

    if (type == msg_type::SET_ENROUTE_HOURS)
    {
        CFlightPlan fp = lookupFp(callsign);
        if (!fp.IsValid()) return MakeError(id, "Flight plan not found: " + callsign);
        CFlightPlanData fpd = fp.GetFlightPlanData();
        bool ok = fpd.SetEnrouteHours(value.c_str());
        return MakeSuccess(id, ok ? "{}" : "");
    }

    if (type == msg_type::SET_ENROUTE_MINUTES)
    {
        CFlightPlan fp = lookupFp(callsign);
        if (!fp.IsValid()) return MakeError(id, "Flight plan not found: " + callsign);
        CFlightPlanData fpd = fp.GetFlightPlanData();
        bool ok = fpd.SetEnrouteMinutes(value.c_str());
        return MakeSuccess(id, ok ? "{}" : "");
    }

    if (type == msg_type::SET_FUEL_HOURS)
    {
        CFlightPlan fp = lookupFp(callsign);
        if (!fp.IsValid()) return MakeError(id, "Flight plan not found: " + callsign);
        CFlightPlanData fpd = fp.GetFlightPlanData();
        bool ok = fpd.SetFuelHours(value.c_str());
        return MakeSuccess(id, ok ? "{}" : "");
    }

    if (type == msg_type::SET_FUEL_MINUTES)
    {
        CFlightPlan fp = lookupFp(callsign);
        if (!fp.IsValid()) return MakeError(id, "Flight plan not found: " + callsign);
        CFlightPlanData fpd = fp.GetFlightPlanData();
        bool ok = fpd.SetFuelMinutes(value.c_str());
        return MakeSuccess(id, ok ? "{}" : "");
    }

    // ========================================================================
    // Action handlers
    // ========================================================================

    if (type == msg_type::AMEND_FLIGHT_PLAN)
    {
        CFlightPlan fp = lookupFp(callsign);
        if (!fp.IsValid()) return MakeError(id, "Flight plan not found: " + callsign);
        CFlightPlanData fpd = fp.GetFlightPlanData();
        bool ok = fpd.AmendFlightPlan();
        return MakeSuccess(id, ok ? "{}" : "");
    }

    if (type == msg_type::START_TRACKING)
    {
        CFlightPlan fp = lookupFp(callsign);
        if (!fp.IsValid()) return MakeError(id, "Flight plan not found: " + callsign);
        bool ok = fp.StartTracking();
        return MakeSuccess(id, ok ? "{}" : "");
    }

    if (type == msg_type::END_TRACKING)
    {
        CFlightPlan fp = lookupFp(callsign);
        if (!fp.IsValid()) return MakeError(id, "Flight plan not found: " + callsign);
        bool ok = fp.EndTracking();
        return MakeSuccess(id, ok ? "{}" : "");
    }

    if (type == msg_type::ACCEPT_HANDOFF)
    {
        CFlightPlan fp = lookupFp(callsign);
        if (!fp.IsValid()) return MakeError(id, "Flight plan not found: " + callsign);
        fp.AcceptHandoff();
        return MakeSuccess(id);
    }

    if (type == msg_type::REFUSE_HANDOFF)
    {
        CFlightPlan fp = lookupFp(callsign);
        if (!fp.IsValid()) return MakeError(id, "Flight plan not found: " + callsign);
        fp.RefuseHandoff();
        return MakeSuccess(id);
    }

    if (type == msg_type::INITIATE_HANDOFF)
    {
        CFlightPlan fp = lookupFp(callsign);
        if (!fp.IsValid()) return MakeError(id, "Flight plan not found: " + callsign);
        std::string target = JStr(data, "target");
        if (target.empty()) return MakeError(id, "Missing 'target' controller");
        bool ok = fp.InitiateHandoff(target.c_str());
        return MakeSuccess(id, ok ? "{}" : "");
    }

    if (type == msg_type::PUSH_FLIGHT_STRIP)
    {
        CFlightPlan fp = lookupFp(callsign);
        if (!fp.IsValid()) return MakeError(id, "Flight plan not found: " + callsign);
        std::string target = JStr(data, "target");
        if (target.empty()) return MakeError(id, "Missing 'target' controller");
        fp.PushFlightStrip(target.c_str());
        return MakeSuccess(id);
    }

    if (type == msg_type::SET_ESTIMATION)
    {
        CFlightPlan fp = lookupFp(callsign);
        if (!fp.IsValid()) return MakeError(id, "Flight plan not found: " + callsign);
        std::string pointName = JStr(data, "point");
        std::string timeVal = JStr(data, "time");
        if (pointName.empty() || timeVal.empty())
            return MakeError(id, "Missing 'point' or 'time'");
        fp.SetEstimation(pointName.c_str(), timeVal.c_str());
        return MakeSuccess(id);
    }

     if (type == msg_type::CLEAR_ESTIMATION)
     {
         //CFlightPlan fp = lookupFp(callsign);
         //if (!fp.IsValid()) return MakeError(id, "Flight plan not found: " + callsign);
         //std::string point = JStr(data, "point");
         //if (point.empty())
         //    fp.ClearEstimation();
         //else
         //    fp.ClearEstimation(point.c_str());
         //return MakeSuccess(id);
         return MakeError(id,"SDK version not supported.");
     }

    if (type == msg_type::SET_FLIGHT_STRIP_ANNOTATION)
    {
        CFlightPlan fp = lookupFp(callsign);
        if (!fp.IsValid()) return MakeError(id, "Flight plan not found: " + callsign);
        int idx = JInt(data, "index", -1);
        std::string annotation = JStr(data, "annotation");
        if (idx < 0 || idx > 8)
            return MakeError(id, "Missing or invalid 'index' (0-8 required)");
        CFlightPlanControllerAssignedData cad = fp.GetControllerAssignedData();
        bool ok = cad.SetFlightStripAnnotation(idx, annotation.c_str());
        return MakeSuccess(id, ok ? "{}" : "");
    }

    // ========================================================================
    // Correlate / Uncorrelate
    // ========================================================================

    if (type == msg_type::CORRELATE)
    {
        std::string fpCallsign = JStr(data, "fp_callsign");
        std::string rtCallsign = JStr(data, "rt_callsign");
        if (fpCallsign.empty() || rtCallsign.empty())
            return MakeError(id, "Missing 'fp_callsign' or 'rt_callsign'");

        CFlightPlan fp = plugin.FlightPlanSelect(fpCallsign.c_str());
        if (!fp.IsValid()) return MakeError(id, "Flight plan not found: " + fpCallsign);

        CRadarTarget rt = plugin.RadarTargetSelect(rtCallsign.c_str());
        if (!rt.IsValid()) return MakeError(id, "Radar target not found: " + rtCallsign);

        bool ok = rt.CorrelateWithFlightPlan(fp);
        return MakeSuccess(id, ok ? "{}" : "");
    }

    if (type == msg_type::UNCORRELATE)
    {
        // Try flight plan first, then radar target
        std::string targetType = JStr(data, "target_type", "fp");
        if (targetType == "rt" || targetType == "radar_target")
        {
            CRadarTarget rt = plugin.RadarTargetSelect(callsign.c_str());
            if (!rt.IsValid()) return MakeError(id, "Radar target not found: " + callsign);
            rt.Uncorrelate();
        }
        else
        {
            CFlightPlan fp = lookupFp(callsign);
            if (!fp.IsValid()) return MakeError(id, "Flight plan not found: " + callsign);
            fp.Uncorrelate();
        }
        return MakeSuccess(id);
    }

    // ========================================================================
    // Set ASEL
    // ========================================================================

    if (type == msg_type::SET_ASEL)
    {
        if (callsign.empty())
            return MakeError(id, "Missing 'callsign'");

        std::string aselType = JStr(data, "asel_type", "fp");
        if (aselType == "rt" || aselType == "radar_target")
        {
            CRadarTarget rt = plugin.RadarTargetSelect(callsign.c_str());
            if (!rt.IsValid()) return MakeError(id, "Radar target not found: " + callsign);
            plugin.SetASELAircraft(rt);
        }
        else
        {
            CFlightPlan fp = lookupFp(callsign);
            if (!fp.IsValid()) return MakeError(id, "Flight plan not found: " + callsign);
            plugin.SetASELAircraft(fp);
        }
        return MakeSuccess(id);
    }

    // ========================================================================
    // Display message
    // ========================================================================

    if (type == msg_type::DISPLAY_MESSAGE)
    {
        std::string handler = JStr(data, "handler", "DataBridge");
        std::string sender  = JStr(data, "sender");
        std::string message = JStr(data, "message");
        bool showHandler    = JBool(data, "show", true);
        bool showUnread     = JBool(data, "unread", true);
        bool showEvenIfBusy = JBool(data, "busy", true);
        bool flash          = JBool(data, "flash", true);
        bool confirm        = JBool(data, "confirm", false);

        plugin.DisplayUserMessage(
            handler.c_str(), sender.c_str(), message.c_str(),
            showHandler, showUnread, showEvenIfBusy, flash, confirm);
        return MakeSuccess(id);
    }

    // ========================================================================
    // Send raw command to EuroScope
    // ========================================================================

    if (type == msg_type::SEND_COMMAND)
    {
        std::string cmd = JStr(data, "command");
        if (cmd.empty())
            return MakeError(id, "Missing 'command'");
        // Note: OnCompileCommand is virtual and returns a bool;
        // the bridge returns false to let EuroScope handle it.
        // We send the result back.
        // Actually, EuroScope calls OnCompileCommand on the plugin when a command
        // is typed - we can't programmatically "send" a command. But we return
        // a response indicating the command was noted.
        json result;
        result["command"] = cmd;
        return MakeSuccess(id, result.dump());
    }

    // ========================================================================
    // Voice channel toggles
    // ========================================================================

    if (type == msg_type::TOGGLE_PRIMARY || type == msg_type::TOGGLE_ATIS ||
        type == msg_type::TOGGLE_TEXT_RECEIVE || type == msg_type::TOGGLE_TEXT_TRANSMIT ||
        type == msg_type::TOGGLE_VOICE_RECEIVE || type == msg_type::TOGGLE_VOICE_TRANSMIT)
    {
        std::string channelName = JStr(data, "channel");
        CGrountToAirChannel targetChannel;
        bool found = false;

        if (!channelName.empty())
        {
            CGrountToAirChannel ch = plugin.GroundToArChannelSelectFirst();
            while (ch.IsValid())
            {
                std::string name = SafeStr(ch.GetName());
                if (name == channelName)
                {
                    targetChannel = ch;
                    found = true;
                    break;
                }
                ch = plugin.GroundToArChannelSelectNext(ch);
            }
        }
        else
        {
            // If no channel specified, use the first valid one
            CGrountToAirChannel ch = plugin.GroundToArChannelSelectFirst();
            if (ch.IsValid())
            {
                targetChannel = ch;
                found = true;
            }
        }

        if (!found || !targetChannel.IsValid())
            return MakeError(id, "Voice channel not found: " + channelName);

        if (type == msg_type::TOGGLE_PRIMARY)           targetChannel.TogglePrimary();
        else if (type == msg_type::TOGGLE_ATIS)          targetChannel.ToggleAtis();
        else if (type == msg_type::TOGGLE_TEXT_RECEIVE)   targetChannel.ToggleTextReceive();
        else if (type == msg_type::TOGGLE_TEXT_TRANSMIT)  targetChannel.ToggleTextTransmit();
        else if (type == msg_type::TOGGLE_VOICE_RECEIVE)  targetChannel.ToggleVoiceReceive();
        else if (type == msg_type::TOGGLE_VOICE_TRANSMIT) targetChannel.ToggleVoiceTransmit();

        return MakeSuccess(id);
    }

    // ========================================================================
    // Unknown type
    // ========================================================================

    return MakeError(id, "Unknown message type: " + type);
}

} // namespace edb
