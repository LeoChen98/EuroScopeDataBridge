#include "Serializer.h"

#include <nlohmann/json.hpp>

#include <string>
#include <vector>

using json = nlohmann::json;
using namespace EuroScopePlugIn;

namespace edb {

// ============================================================================
// FlightPlanPositionPredictions
// ============================================================================

std::string SerializePositionPredictionsToJson(CFlightPlanPositionPredictions& pred)
{
    json j;
    int count = pred.GetPointsNumber();
    j["points_number"] = count;

    json points = json::array();
    for (int i = 0; i < count; ++i)
    {
        json pt;
        CPosition pos = pred.GetPosition(i);
        pt["index"] = i;
        pt["latitude"] = pos.m_Latitude;
        pt["longitude"] = pos.m_Longitude;
        pt["altitude"] = pred.GetAltitude(i);
        const char* ctrlId = pred.GetControllerId(i);
        pt["controller_id"] = SafeStr(ctrlId);
        points.push_back(std::move(pt));
    }
    j["points"] = std::move(points);

    return j.dump();
}

// ============================================================================
// RadarTargetPositionData
// ============================================================================

std::string SerializeRadarTargetPositionDataToJson(CRadarTargetPositionData& pos)
{
    json j;

    if (!pos.IsValid())
    {
        j["valid"] = false;
        return j.dump();
    }

    j["valid"] = true;

    CPosition cp = pos.GetPosition();
    j["latitude"] = cp.m_Latitude;
    j["longitude"] = cp.m_Longitude;

    j["flight_level"] = pos.GetFlightLevel();
    j["squawk"] = SafeStr(pos.GetSquawk());
    j["reported_gs"] = pos.GetReportedGS();
    j["reported_heading"] = pos.GetReportedHeading();
    j["reported_heading_true_north"] = pos.GetReportedHeadingTrueNorth();
    j["reported_pitch"] = pos.GetReportedPitch();
    j["reported_bank"] = pos.GetReportedBank();
    j["radar_flags"] = pos.GetRadarFlags();
    j["received_time"] = pos.GetReceivedTime();
    j["transponder_c"] = pos.GetTransponderC();
    j["transponder_i"] = pos.GetTransponderI();

    return j.dump();
}

// ============================================================================
// FlightPlanData
// ============================================================================

std::string SerializeFlightPlanDataToJson(CFlightPlanData& fpd)
{
    json j;

    j["is_received"] = fpd.IsReceived();
    j["is_amended"] = fpd.IsAmended();

    j["plan_type"] = SafeStr(fpd.GetPlanType());
    j["aircraft_info"] = SafeStr(fpd.GetAircraftInfo());

    // Aircraft characteristics
    char wtc = fpd.GetAircraftWtc();
    j["aircraft_wtc"] = std::string(1, wtc);

    char acType = fpd.GetAircraftType();
    j["aircraft_type"] = std::string(1, acType);

    j["engine_number"] = fpd.GetEngineNumber();

    char engType = fpd.GetEngineType();
    j["engine_type"] = std::string(1, engType);

    char cap = fpd.GetCapibilities();
    j["capabilities"] = std::string(1, cap);

    j["is_rvsm"] = fpd.IsRvsm();
    j["manufacturer_type"] = SafeStr(fpd.GetManufacturerType());
    j["aircraft_fp_type"] = SafeStr(fpd.GetAircraftFPType());
    j["true_airspeed"] = fpd.GetTrueAirspeed();

    // Route
    j["origin"]           = SafeStr(fpd.GetOrigin());
    j["destination"]      = SafeStr(fpd.GetDestination());
    j["alternate"]        = SafeStr(fpd.GetAlternate());
    j["final_altitude"]   = fpd.GetFinalAltitude();  // CFlightPlanData version
    j["remarks"]          = SafeStr(fpd.GetRemarks());

    char commType = fpd.GetCommunicationType();
    j["communication_type"] = std::string(1, commType);

    j["route"]            = SafeStr(fpd.GetRoute());
    j["sid_name"]         = SafeStr(fpd.GetSidName());
    j["star_name"]        = SafeStr(fpd.GetStarName());
    j["departure_rwy"]    = SafeStr(fpd.GetDepartureRwy());
    j["arrival_rwy"]      = SafeStr(fpd.GetArrivalRwy());

    // Times
    j["estimated_departure_time"] = SafeStr(fpd.GetEstimatedDepartureTime());
    j["actual_departure_time"]    = SafeStr(fpd.GetActualDepartureTime());
    j["enroute_hours"]            = SafeStr(fpd.GetEnrouteHours());
    j["enroute_minutes"]          = SafeStr(fpd.GetEnrouteMinutes());
    j["fuel_hours"]               = SafeStr(fpd.GetFuelHours());
    j["fuel_minutes"]             = SafeStr(fpd.GetFuelMinutes());

    return j.dump();
}

// ============================================================================
// FlightPlanControllerAssignedData
// ============================================================================

std::string SerializeControllerAssignedDataToJson(CFlightPlanControllerAssignedData& cad)
{
    json j;

    j["squawk"]              = SafeStr(cad.GetSquawk());
    j["final_altitude"]      = cad.GetFinalAltitude();
    j["cleared_altitude"]    = cad.GetClearedAltitude();

    char commType = cad.GetCommunicationType();
    j["communication_type"]  = std::string(1, commType);

    j["scratchpad"]          = SafeStr(cad.GetScratchPadString());
    j["assigned_speed"]      = cad.GetAssignedSpeed();
    j["assigned_mach"]       = cad.GetAssignedMach();
    j["assigned_rate"]       = cad.GetAssignedRate();
    j["assigned_heading"]    = cad.GetAssignedHeading();
    j["direct_to"]           = SafeStr(cad.GetDirectToPointName());

    // Flight strip annotations (0-8)
    json annotations = json::array();
    for (int i = 0; i <= 8; ++i)
    {
        const char* ann = cad.GetFlightStripAnnotation(i);
        annotations.push_back(SafeStr(ann));
    }
    j["flight_strip_annotations"] = std::move(annotations);

    return j.dump();
}

// ============================================================================
// FlightPlanExtractedRoute
// ============================================================================

std::string SerializeExtractedRouteToJson(CFlightPlanExtractedRoute& route)
{
    json j;

    int count = route.GetPointsNumber();
    j["points_number"] = count;
    j["calculated_index"] = route.GetPointsCalculatedIndex();
    j["assigned_index"] = route.GetPointsAssignedIndex();

    json points = json::array();
    for (int i = 0; i < count; ++i)
    {
        json pt;
        pt["index"] = i;
        pt["name"] = SafeStr(route.GetPointName(i));

        CPosition pos = route.GetPointPosition(i);
        pt["latitude"] = pos.m_Latitude;
        pt["longitude"] = pos.m_Longitude;

        pt["airway_name"] = SafeStr(route.GetPointAirwayName(i));
        pt["airway_classification"] = route.GetPointAirwayClassification(i);
        pt["distance_in_minutes"] = route.GetPointDistanceInMinutes(i);
        pt["calculated_profile_altitude"] = route.GetPointCalculatedProfileAltitude(i);

        points.push_back(std::move(pt));
    }
    j["points"] = std::move(points);

    return j.dump();
}

// ============================================================================
// CFlightPlan (top-level)
// ============================================================================

std::string SerializeFlightPlanToJson(CFlightPlan& fp)
{
    json j;

    if (!fp.IsValid())
    {
        j["valid"] = false;
        return j.dump();
    }

    j["valid"] = true;

    // -- Identity --
    j["callsign"]              = SafeStr(fp.GetCallsign());
    j["pilot_name"]            = SafeStr(fp.GetPilotName());
    j["state"]                 = fp.GetState();
    j["fp_state"]              = fp.GetFPState();
    j["simulated"]             = fp.GetSimulated();

    // -- Tracking --
    j["tracking_controller_callsign"] = SafeStr(fp.GetTrackingControllerCallsign());
    j["tracking_controller_id"]       = SafeStr(fp.GetTrackingControllerId());
    j["tracking_controller_is_me"]    = fp.GetTrackingControllerIsMe();

    // -- Handoff --
    j["handoff_target_callsign"] = SafeStr(fp.GetHandoffTargetControllerCallsign());
    j["handoff_target_id"]       = SafeStr(fp.GetHandoffTargetControllerId());

    // -- Route --
    j["distance_to_destination"] = fp.GetDistanceToDestination();
    j["distance_from_origin"]    = fp.GetDistanceFromOrigin();
    j["next_copx"]               = SafeStr(fp.GetNextCopxPointName());
    j["next_fir_copx"]           = SafeStr(fp.GetNextFirCopxPointName());
    j["sector_entry_minutes"]    = fp.GetSectorEntryMinutes();
    j["sector_exit_minutes"]     = fp.GetSectorExitMinutes();

    // -- Flags --
    j["ram_flag"]            = fp.GetRAMFlag();
    j["clam_flag"]           = fp.GetCLAMFlag();
    j["ground_state"]        = SafeStr(fp.GetGroundState());
    j["clearence_flag"]      = fp.GetClearenceFlag();
    j["text_communication"]  = fp.IsTextCommunication();

    // -- Altitudes --
    j["final_altitude"]      = fp.GetFinalAltitude();
    j["cleared_altitude"]    = fp.GetClearedAltitude();

    // -- Coordination (Entry) --
    j["entry_coordination_point_name"]   = SafeStr(fp.GetEntryCoordinationPointName());
    j["entry_coordination_point_state"]  = fp.GetEntryCoordinationPointState();
    j["entry_coordination_altitude_state"] = fp.GetEntryCoordinationAltitudeState();
    j["entry_coordination_altitude"]     = fp.GetEntryCoordinationAltitude();

    // -- Coordination (Exit) --
    j["exit_coordination_point_name"]   = SafeStr(fp.GetExitCoordinationPointName());
    j["exit_coordination_name_state"]   = fp.GetExitCoordinationNameState();
    j["exit_coordination_altitude_state"] = fp.GetExitCoordinationAltitudeState();
    j["exit_coordination_altitude"]     = fp.GetExitCoordinationAltitude();

    // -- Next Controller --
    j["coordinated_next_controller"]        = SafeStr(fp.GetCoordinatedNextController());
    j["coordinated_next_controller_state"]  = fp.GetCoordinatedNextControllerState();

    // -- Sub-objects (temporary, serialize inline) --
    // Each sub-object is serialized defensively: a failure in one must not
    // abort serialization of the whole flight plan (CFlightPlanExtractedRoute
    // and CFlightPlanPositionPredictions expose no IsValid() to pre-check).
    // Client-visible effect: a failed sub-object serializes as an empty object.
    {
        CFlightPlanData fpd = fp.GetFlightPlanData();
        try {
            j["flight_plan_data"] = json::parse(SerializeFlightPlanDataToJson(fpd));
        } catch (...) {
            j["flight_plan_data"] = json::object();
        }
    }

    {
        CFlightPlanControllerAssignedData cad = fp.GetControllerAssignedData();
        try {
            j["controller_assigned_data"] = json::parse(SerializeControllerAssignedDataToJson(cad));
        } catch (...) {
            j["controller_assigned_data"] = json::object();
        }
    }

    {
        CFlightPlanExtractedRoute route = fp.GetExtractedRoute();
        try {
            j["extracted_route"] = json::parse(SerializeExtractedRouteToJson(route));
        } catch (...) {
            j["extracted_route"] = json::object();
        }
    }

    {
        CFlightPlanPositionPredictions pred = fp.GetPositionPredictions();
        try {
            j["position_predictions"] = json::parse(SerializePositionPredictionsToJson(pred));
        } catch (...) {
            j["position_predictions"] = json::object();
        }
    }

    {
        CRadarTargetPositionData trackPos = fp.GetFPTrackPosition();
        try {
            j["track_position"] = json::parse(SerializeRadarTargetPositionDataToJson(trackPos));
        } catch (...) {
            j["track_position"] = json::object();
        }
    }

    return j.dump();
}

// ============================================================================
// CRadarTarget
// ============================================================================

std::string SerializeRadarTargetToJson(CRadarTarget& rt)
{
    json j;

    if (!rt.IsValid())
    {
        j["valid"] = false;
        return j.dump();
    }

    j["valid"] = true;
    j["callsign"]       = SafeStr(rt.GetCallsign());
    j["system_id"]      = SafeStr(rt.GetSystemID());
    j["vertical_speed"] = rt.GetVerticalSpeed();
    j["track_heading"]  = rt.GetTrackHeading();
    j["ground_speed"]   = rt.GetGS();

    // Correlated flight plan (only serialize if valid)
    CFlightPlan correlated = rt.GetCorrelatedFlightPlan();
    if (correlated.IsValid())
    {
        j["correlated_callsign"] = SafeStr(correlated.GetCallsign());
    }
    else
    {
        j["correlated_callsign"] = nullptr;
    }

    // Position data (temporary, serialize inline) — isolated so a failure in
    // the SDK position data cannot abort serialization of the radar target.
    {
        CRadarTargetPositionData pos = rt.GetPosition();
        try {
            j["position"] = json::parse(SerializeRadarTargetPositionDataToJson(pos));
        } catch (...) {
            j["position"] = json::object();
        }

        // Position history (walk back through previous positions)
        json history = json::array();
        CRadarTargetPositionData prev = rt.GetPreviousPosition(pos);
        while (prev.IsValid())
        {
            try {
                history.push_back(json::parse(SerializeRadarTargetPositionDataToJson(prev)));
            } catch (...) {
                // skip this history entry and keep walking
            }
            prev = rt.GetPreviousPosition(prev);
        }
        j["position_history"] = std::move(history);
    }

    return j.dump();
}

// ============================================================================
// CController
// ============================================================================

std::string SerializeControllerToJson(CController& ctr)
{
    json j;

    if (!ctr.IsValid())
    {
        j["valid"] = false;
        return j.dump();
    }

    j["valid"] = true;
    j["callsign"]           = SafeStr(ctr.GetCallsign());
    j["position_id"]        = SafeStr(ctr.GetPositionId());
    j["identified"]         = ctr.GetPositionIdentified();
    j["primary_frequency"]  = ctr.GetPrimaryFrequency();
    j["full_name"]          = SafeStr(ctr.GetFullName());
    j["rating"]             = ctr.GetRating();
    j["facility"]           = ctr.GetFacility();
    j["sector_file"]        = SafeStr(ctr.GetSectorFileName());
    j["is_controller"]      = ctr.IsController();

    CPosition pos = ctr.GetPosition();
    j["position"]["latitude"]  = pos.m_Latitude;
    j["position"]["longitude"] = pos.m_Longitude;

    j["range"]        = ctr.GetRange();
    j["is_breaking"]  = ctr.IsBreaking();
    j["ongoing_able"] = ctr.IsOngoingAble();

    return j.dump();
}

// ============================================================================
// CSectorElement
// ============================================================================

std::string SerializeSectorElementToJson(CSectorElement& se)
{
    json j;

    if (!se.IsValid())
    {
        j["valid"] = false;
        return j.dump();
    }

    j["valid"] = true;
    j["name"]         = SafeStr(se.GetName());
    j["element_type"] = se.GetElementType();

    // Positions (iterate until GetPosition returns false)
    json positions = json::array();
    for (int i = 0; ; ++i)
    {
        CPosition pos;
        if (!se.GetPosition(&pos, i))
            break;
        json pt;
        pt["latitude"]  = pos.m_Latitude;
        pt["longitude"] = pos.m_Longitude;
        positions.push_back(std::move(pt));
    }
    j["positions"] = std::move(positions);

    // Components
    json components = json::array();
    for (int i = 0; ; ++i)
    {
        const char* comp = se.GetComponentName(i);
        if (!comp || comp[0] == '\0')
            break;
        components.push_back(comp);
    }
    j["components"] = std::move(components);

    j["frequency"] = se.GetFrequency();

    json runways = json::array();
    for (int i = 0; i <= 1; ++i)
    {
        const char* rwyName = se.GetRunwayName(i);
        int rwyHdg = se.GetRunwayHeading(i);
        if (rwyName && rwyName[0] != '\0')
        {
            json rwy;
            rwy["name"]    = rwyName;
            rwy["heading"] = rwyHdg;
            runways.push_back(std::move(rwy));
        }
    }
    j["runways"] = std::move(runways);

    j["airport_name"] = SafeStr(se.GetAirportName());
    j["active_arrival"]   = se.IsElementActive(false);
    j["active_departure"] = se.IsElementActive(true);

    return j.dump();
}

// ============================================================================
// CGrountToAirChannel
// ============================================================================

std::string SerializeVoiceChannelToJson(CGrountToAirChannel& ch)
{
    json j;

    if (!ch.IsValid())
    {
        j["valid"] = false;
        return j.dump();
    }

    j["valid"] = true;
    j["name"]              = SafeStr(ch.GetName());
    j["frequency"]         = ch.GetFrequency();
    j["voice_server"]      = SafeStr(ch.GetVoiceServer());
    j["voice_channel"]     = SafeStr(ch.GetVoiceChannel());
    j["is_primary"]        = ch.GetIsPrimary();
    j["is_atis"]           = ch.GetIsAtis();
    j["is_text_receive_on"]  = ch.GetIsTextReceiveOn();
    j["is_text_transmit_on"] = ch.GetIsTextTransmitOn();
    j["is_voice_receive_on"]  = ch.GetIsVoiceReceiveOn();
    j["is_voice_transmit_on"] = ch.GetIsVoiceTransmitOn();
    j["is_voice_connected"]   = ch.GetIsVoiceConnected();

    return j.dump();
}

} // namespace edb
