#pragma once

// ============================================================================
// EuroScope Data Bridge — Constants
// ============================================================================

// --- Plugin Identity ---
#define   PLUGIN_NAME         "EuroScope Data Bridge"
#define   PLUGIN_VERSION      "1.1.1"
#define   PLUGIN_AUTHOR       "Leo Chen"
#define   PLUGIN_COPYRIGHT    "MIT License"

// Numeric version parts (used by the Win32 version resource, Resource.rc).
#define   PLUGIN_VERSION_MAJOR   1
#define   PLUGIN_VERSION_MINOR   1
#define   PLUGIN_VERSION_PATCH   1

namespace edb {

// --- WebSocket Configuration ---
constexpr int    WS_PORT                 = 48521;


// Max outbound bytes buffered per client before the server drops new frames
// (backpressure guard against unbounded memory growth / bad_alloc; enforced in
// WebSocketServer::IsSendQueueFull via connection::get_buffered_amount()).
constexpr size_t MAX_CLIENT_SEND_QUEUE_BYTES = 32 * 1024 * 1024;  // 32 MB

// Frame-count cap. NOT enforced at runtime: websocketpp exposes no public API
// to query the number of queued frames, so backpressure is byte-based only.
constexpr size_t MAX_CLIENT_SEND_QUEUE_FRAMES = 8192;

// Cadence of the EuroScope OnTimer callback (main thread); incoming client
// requests are drained and processed on that beat.
constexpr int TIMER_INTERVAL_MS = 1000;

// Max inbound WebSocket message size accepted from a client (bytes). Larger
// messages are rejected with the WebSocket "message_too_big" protocol error.
constexpr size_t MAX_INCOMING_MESSAGE_BYTES = 1 * 1024 * 1024;  // 1 MB

// Max pending inbound requests buffered between the WebSocket layer and the
// EuroScope main thread. When full, the oldest request is dropped.
constexpr size_t MAX_INCOMING_QUEUE_SIZE = 1024;

// --- Connection Limits ---
constexpr int    MAX_CLIENTS             = 64;    // hard cap on concurrent WebSocket clients

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
    constexpr auto AIRPORT_RUNWAY_ACTIVITY_CHANGED = "airport_runway_activity_changed";

    // Request (client → server) — Queries
    constexpr auto GET_FLIGHTPLANS               = "get_flightplans";
    constexpr auto GET_RADAR_TARGETS             = "get_radar_targets";
    constexpr auto GET_CONTROLLERS               = "get_controllers";
    constexpr auto GET_SECTOR_ELEMENTS           = "get_sector_elements";
    constexpr auto GET_VOICE_CHANNELS            = "get_voice_channels";
    constexpr auto GET_TRANSITION_ALTITUDE       = "get_transition_altitude";
    constexpr auto GET_CONNECTION_TYPE           = "get_connection_type";
    constexpr auto GET_FULL_SNAPSHOT              = "get_full_snapshot";

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

    // Request (client → server) — Subscription control
    constexpr auto SUBSCRIBE                     = "subscribe";
    constexpr auto UNSUBSCRIBE                   = "unsubscribe";

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

    // Internal routing tag
    constexpr auto BROADCAST                     = "_broadcast";
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
    constexpr auto EVENTS   = "events";

    // Routing (internal, not exposed to clients)
    constexpr auto CLIENT_ID = "client_id";
    constexpr auto ROUTE     = "_route";
}

} // namespace edb
