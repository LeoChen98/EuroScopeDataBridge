#pragma once

// ============================================================================
// EuroScope Data Bridge — Constants
// ============================================================================

namespace edb {

// --- Plugin Identity ---
constexpr int    COMPATIBILITY_CODE = 16;
constexpr auto   PLUGIN_NAME        = "EuroScope Data Bridge";
constexpr auto   PLUGIN_VERSION     = "1.0.0";
constexpr auto   PLUGIN_AUTHOR      = "DataBridge";
constexpr auto   PLUGIN_COPYRIGHT   = "MIT License";

// --- WebSocket Configuration ---
constexpr int    WS_PORT            = 48521;
constexpr int    WS_OUTGOING_INTERVAL_MS = 10;   // outgoing queue flush interval

// --- Timer Configuration ---
constexpr int    FULL_SNAPSHOT_INTERVAL = 10;    // send full snapshot every N seconds

// ============================================================================
// Message Type String Constants (Push)
// ============================================================================
#ifdef ERROR
#undef ERROR
#endif

namespace msg_type {
    // Push events (server → client)
    constexpr auto RADAR_UPDATE                  = "radar_update";
    constexpr auto FLIGHTPLAN_UPDATE             = "flightplan_update";
    constexpr auto FLIGHTPLAN_DISCONNECT         = "flightplan_disconnect";
    constexpr auto CONTROLLER_UPDATE             = "controller_update";
    constexpr auto CONTROLLER_DISCONNECT         = "controller_disconnect";
    constexpr auto CONTROLLER_ASSIGNED_DATA      = "controller_assigned_data";
    constexpr auto FLIGHT_STRIP_PUSHED           = "flight_strip_pushed";
    constexpr auto CHAT_PRIVATE                  = "chat_private";
    constexpr auto CHAT_FREQUENCY                = "chat_frequency";
    constexpr auto METAR_RECEIVED                = "metar_received";
    constexpr auto PLANE_INFO                    = "plane_info";
    constexpr auto TIMER                         = "timer";
    constexpr auto FULL_SNAPSHOT                 = "full_snapshot";

    // Request (client → server) — Queries
    constexpr auto GET_FLIGHTPLANS               = "get_flightplans";
    constexpr auto GET_RADAR_TARGETS             = "get_radar_targets";
    constexpr auto GET_CONTROLLERS               = "get_controllers";
    constexpr auto GET_SECTOR_ELEMENTS           = "get_sector_elements";
    constexpr auto GET_VOICE_CHANNELS            = "get_voice_channels";
    constexpr auto GET_TRANSITION_ALTITUDE       = "get_transition_altitude";
    constexpr auto GET_CONNECTION_TYPE           = "get_connection_type";

    // Request (client → server) — Setters
    constexpr auto SET_SQUAWK                    = "set_squawk";
    constexpr auto SET_FINAL_ALTITUDE            = "set_final_altitude";
    constexpr auto SET_CLEARED_ALTITUDE          = "set_cleared_altitude";
    constexpr auto SET_SCRATCHPAD                = "set_scratchpad";
    constexpr auto SET_SPEED                     = "set_speed";
    constexpr auto SET_MACH                      = "set_mach";
    constexpr auto SET_RATE                      = "set_rate";
    constexpr auto SET_HEADING                   = "set_heading";
    constexpr auto SET_DIRECT_TO                 = "set_direct_to";
    constexpr auto SET_COMMUNICATION_TYPE        = "set_communication_type";
    constexpr auto SET_GROUND_STATE              = "set_ground_state";
    constexpr auto SET_CLEARENCE_FLAG            = "set_clearence_flag";
    constexpr auto SET_PLAN_TYPE                 = "set_plan_type";
    constexpr auto SET_AIRCRAFT_INFO             = "set_aircraft_info";
    constexpr auto CORRELATE                     = "correlate";
    constexpr auto UNCORRELATE                   = "uncorrelate";
    constexpr auto SET_ASEL                      = "set_asel";
    constexpr auto DISPLAY_MESSAGE               = "display_message";
    constexpr auto SEND_COMMAND                  = "send_command";

    // Request (client → server) — Extra Setters (from .h, not in original SDD)
    constexpr auto SET_ORIGIN                    = "set_origin";
    constexpr auto SET_DESTINATION               = "set_destination";
    constexpr auto SET_ALTERNATE                 = "set_alternate";
    constexpr auto SET_REMARKS                   = "set_remarks";
    constexpr auto SET_ROUTE                     = "set_route";
    constexpr auto SET_TRUE_AIRSPEED             = "set_true_airspeed";
    constexpr auto SET_DEPARTURE_TIME            = "set_departure_time";
    constexpr auto SET_ACTUAL_DEPARTURE_TIME     = "set_actual_departure_time";
    constexpr auto SET_ENROUTE_HOURS             = "set_enroute_hours";
    constexpr auto SET_ENROUTE_MINUTES           = "set_enroute_minutes";
    constexpr auto SET_FUEL_HOURS                = "set_fuel_hours";
    constexpr auto SET_FUEL_MINUTES              = "set_fuel_minutes";
    constexpr auto SET_FLIGHT_STRIP_ANNOTATION   = "set_flight_strip_annotation";
    constexpr auto AMEND_FLIGHT_PLAN             = "amend_flight_plan";
    constexpr auto START_TRACKING                = "start_tracking";
    constexpr auto END_TRACKING                  = "end_tracking";
    constexpr auto ACCEPT_HANDOFF                = "accept_handoff";
    constexpr auto REFUSE_HANDOFF                = "refuse_handoff";
    constexpr auto INITIATE_HANDOFF              = "initiate_handoff";
    constexpr auto PUSH_FLIGHT_STRIP             = "push_flight_strip";
    constexpr auto SET_ESTIMATION                = "set_estimation";
    constexpr auto CLEAR_ESTIMATION              = "clear_estimation";

    // Voice channel toggles
    constexpr auto TOGGLE_PRIMARY                = "toggle_primary";
    constexpr auto TOGGLE_ATIS                   = "toggle_atis";
    constexpr auto TOGGLE_TEXT_RECEIVE           = "toggle_text_receive";
    constexpr auto TOGGLE_TEXT_TRANSMIT          = "toggle_text_transmit";
    constexpr auto TOGGLE_VOICE_RECEIVE          = "toggle_voice_receive";
    constexpr auto TOGGLE_VOICE_TRANSMIT         = "toggle_voice_transmit";

    // Response
    constexpr auto RESPONSE                      = "response";
    constexpr auto ERROR                         = "error";
}

// ============================================================================
// JSON Key Constants
// ============================================================================
namespace json_key {
    constexpr auto TYPE     = "type";
    constexpr auto ID       = "id";
    constexpr auto DATA     = "data";
    constexpr auto CALLSIGN = "callsign";
    constexpr auto VALUE    = "value";
    constexpr auto SUCCESS  = "success";
    constexpr auto ERROR    = "error";
    constexpr auto COUNTER  = "counter";
}

} // namespace edb
