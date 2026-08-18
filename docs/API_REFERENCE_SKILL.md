---
name: edb-websocket-api
description: 面向 AI agent 的 skill——在任意项目中实现 EuroScope Data Bridge WebSocket 客户端/脚本/集成时的权威速查（消息格式、推送事件、请求、数据结构、错误与常量）。AI agent skill: authoritative quick lookup for implementing EuroScope Data Bridge WebSocket clients in any project.
---

# EuroScope Data Bridge — WebSocket API Skill (AI Agent)

This is a skill written for AI agents. When you need to implement a
client, script or integration against the EuroScope Data Bridge
WebSocket API, treat the sections below as the authoritative quick
lookup. For the complete field-level documentation, consult
[docs/wiki.md](wiki.md) (English) or [docs/wiki_CN.md](wiki_CN.md)
(中文) — this skill is a condensed, portable slice of it.

**How to use this skill:**

- Match request/response pairs by the `id` field; a connection may have
  several requests in flight.
- Push events arrive only after the client subscribes; subscriptions
  are per-connection.
- If a request or event below is missing a field you need, look it up
  in the wiki **before inventing names** — the protocol only exposes
  what the plugin serializes.
- The **Known gaps** section documents behaviour that differs from the
  wiki; never assume unimplemented requests work.
- Envelope and error strings must be reproduced exactly as written
  here.

> This file is the AI-agent skill copy distributed with the repository.
> When the API changes, keep this file and the local
> `.agents/skills/edb-websocket-api/SKILL.md` in sync (see the
> `sync-docs-web` skill for the full docs-update workflow).

## Connection

| Item | Value |
|---|---|
| Protocol | WebSocket (ws, no TLS) |
| Endpoint | `ws://127.0.0.1:48521` (loopback only) |
| Messages | JSON text |

## Message envelopes

**Request (client → server):**

```json
{ "type": "<type>", "id": "<optional unique id>", "data": { } }
```

**Push event (server → client, only after subscribing; no `id`):**

```json
{ "type": "<event type>", "data": { } }
```

**Response (server → client, `id` echoes the request):**

```json
{ "type": "response", "id": "<id>", "data": { "success": true,  "result": { } } }
{ "type": "response", "id": "<id>", "data": { "success": false, "error": "..." } }
```

**Immediate error (unparseable/invalid, no `id`):**

```json
{ "type": "error", "data": { "error": "Invalid JSON" } }
```

> `subscribe`/`unsubscribe` replies use `"type": "response"` like every
> other request.

## Heartbeat (optional)

Send `ping` → server replies `pong` immediately (echoes `id`). Answered
inline on the IO thread; does not consume a concurrent-request slot.

```json
→ { "type": "ping", "id": "hb-1" }
← { "type": "pong", "id": "hb-1" }
```

## Subscription

Push events are **not** delivered until the client `subscribe`s.
Subscriptions are per-connection and cleared on disconnect.

```json
→ { "type": "subscribe",   "id": "s1", "data": { "events": ["radar_update", "timer"] } }
→ { "type": "unsubscribe", "id": "s2", "data": { "events": ["timer"] } }
→ { "type": "unsubscribe", "id": "s3" }      // omit events = clear all
← { "type": "response", "id": "s1", "data": { "success": true, "events": ["radar_update", "timer"] } }
```

## Push events (13)

| type | Fired on | Key `data` fields |
|---|---|---|
| `radar_update` | radar position update | RadarTarget object |
| `flightplan_update` | flight plan data update | FlightPlan object |
| `flightplan_disconnect` | flight plan disconnect | `callsign` |
| `controller_update` | controller online / changed | Controller object |
| `controller_disconnect` | controller disconnect | `callsign`, `position_id` |
| `controller_assigned_data` | controller assigned-data change | FlightPlan object + `data_type` (int) |
| `flight_strip_pushed` | flight strip pushed | `callsign`, `sender`, `target` |
| `chat_private` | private chat | `sender`, `receiver`, `message` |
| `chat_frequency` | frequency text chat | `sender`, `frequency`, `message` |
| `metar_received` | new METAR | `station`, `metar` |
| `plane_info` | plane type / livery update | `callsign`, `livery`, `plane_type` |
| `timer` | every second | `counter` (int) |
| `airport_runway_activity_changed` | runway activity change | `{}` |

## Requests (client → server)

### Queries (result = array/object)

| type | Params | Returns |
|---|---|---|
| `get_flightplans` | `callsign`? (omitted = all) | FlightPlan[] |
| `get_radar_targets` | `callsign`? | RadarTarget[] |
| `get_controllers` | — | Controller[] (includes self) |
| `get_sector_elements` | `element_type`? (int, -1 = all) | SectorElement[] |
| `get_voice_channels` | — | VoiceChannel[] |
| `get_transition_altitude` | — | `{ "transition_altitude": int }` |
| `get_connection_type` | — | `{ "connection_type": int }` |
| `get_full_snapshot` | — | `{ "flightplans": [], "radar_targets": [], "controllers": [] }` |

### Setters — ControllerAssignedData (`data.callsign` + `data.value`)

`set_squawk` (string 4-digit), `set_final_altitude` (int ft), `set_cleared_altitude` (int ft), `set_scratchpad` (string), `set_speed` (int kt), `set_mach` (int ×100), `set_rate` (int ft/min), `set_heading` (int 0-360), `set_direct_to` (string waypoint), `set_communication_type` (string V/T/R).

### Setters — FlightPlanData (`data.callsign` + `data.value`)

`set_plan_type` (string), `set_aircraft_info` (string), `set_origin` (string ICAO), `set_destination` (string), `set_alternate` (string), `set_remarks` (string), `set_route` (string), `set_true_airspeed` (int), `set_departure_time` (string), `set_actual_departure_time` (string), `set_enroute_hours` (string), `set_enroute_minutes` (string), `set_fuel_hours` (string), `set_fuel_minutes` (string).

> Successful setters return `{ "success": true, "result": {} }`; failures
> return a `Failed to set <field>` error.

### Actions

| type | Params | Notes |
|---|---|---|
| `amend_flight_plan` | `callsign` | commit FlightPlanData changes |
| `start_tracking` / `end_tracking` | `callsign` | tracking control |
| `accept_handoff` / `refuse_handoff` | `callsign` | handoff control |
| `initiate_handoff` | `callsign`, `target` | initiate handoff |
| `push_flight_strip` | `callsign`, `target` | push flight strip |
| `set_estimation` | `callsign`, `point`, `time` | set estimate time |
| `clear_estimation` | `callsign`, `point`? | ⚠ not implemented (see Known gaps) |
| `set_flight_strip_annotation` | `callsign`, `index` (0-8), `annotation` | strip annotation |
| `correlate` | `fp_callsign`, `rt_callsign` | correlate FP + radar |
| `uncorrelate` | `callsign`, `target_type`? ("fp"/"rt") | uncorrelate |
| `set_asel` | `callsign`, `asel_type`? ("fp"/"rt") | set selected aircraft |
| `display_message` | `handler`?, `sender`?, `message`?, `show`?, `unread`?, `busy`?, `flash`?, `confirm`? | show ES message |
| `send_command` | `command` | ⚠ echoes only (see Known gaps) |

### Voice channel toggles (`data.channel` optional; omitted = first channel)

`toggle_primary`, `toggle_atis`, `toggle_text_receive`, `toggle_text_transmit`, `toggle_voice_receive`, `toggle_voice_transmit`.

## Data structures (key fields)

- **FlightPlan**: `valid`, `callsign`, `pilot_name`, `state`, `fp_state`, `simulated`, `tracking_controller_callsign/id/is_me`, `handoff_target_callsign/id`, `distance_to_destination`, `distance_from_origin`, `next_copx`, `next_fir_copx`, `sector_entry_minutes`, `sector_exit_minutes`, `ram_flag`, `clam_flag`, `ground_state`, `clearence_flag`, `text_communication`, `final_altitude`, `cleared_altitude`, `entry_coordination_*`, `exit_coordination_*`, `coordinated_next_controller(_state)`, `flight_plan_data`, `controller_assigned_data`, `extracted_route`, `position_predictions`, `track_position`.
- **FlightPlanData**: `is_received`, `is_amended`, `plan_type`, `aircraft_info`, `aircraft_wtc`, `aircraft_type`, `engine_number`, `engine_type`, `capabilities`, `is_rvsm`, `manufacturer_type`, `aircraft_fp_type`, `true_airspeed`, `origin`, `destination`, `alternate`, `final_altitude`, `remarks`, `communication_type`, `route`, `sid_name`, `star_name`, `departure_rwy`, `arrival_rwy`, `estimated_departure_time`, `actual_departure_time`, `enroute_hours`, `enroute_minutes`, `fuel_hours`, `fuel_minutes`.
- **ControllerAssignedData**: `squawk`, `final_altitude`, `cleared_altitude`, `communication_type`, `scratchpad`, `assigned_speed`, `assigned_mach`, `assigned_rate`, `assigned_heading`, `direct_to`, `flight_strip_annotations`[9].
- **ExtractedRoute**: `points_number`, `calculated_index`, `assigned_index`, `points[]` (`index`, `name`, `latitude`, `longitude`, `airway_name`, `airway_classification`, `distance_in_minutes`, `calculated_profile_altitude`).
- **PositionPredictions**: `points_number`, `points[]` (`index`, `latitude`, `longitude`, `altitude`, `controller_id`).
- **RadarTargetPositionData**: `valid`, `latitude`, `longitude`, `flight_level`, `squawk`, `reported_gs`, `reported_heading`, `reported_heading_true_north`, `reported_pitch`, `reported_bank`, `radar_flags`, `received_time`, `transponder_c`, `transponder_i`.
- **RadarTarget**: `valid`, `callsign`, `system_id`, `vertical_speed`, `track_heading`, `ground_speed`, `correlated_callsign` (nullable), `position`, `position_history[]`.
- **Controller**: `valid`, `callsign`, `position_id`, `identified`, `primary_frequency`, `full_name`, `rating`, `facility`, `sector_file`, `is_controller`, `position` (`latitude`, `longitude`), `range`, `is_breaking`, `ongoing_able`.
- **SectorElement**: `valid`, `name`, `element_type`, `positions[]`, `components[]`, `frequency`, `runways[]` (`name`, `heading`), `airport_name`, `active_arrival`, `active_departure`.
- **VoiceChannel**: `valid`, `name`, `frequency`, `voice_server`, `voice_channel`, `is_primary`, `is_atis`, `is_text_receive_on`, `is_text_transmit_on`, `is_voice_receive_on`, `is_voice_transmit_on`, `is_voice_connected`.

## Errors

Request errors use the `response` envelope with `"success": false`; the
most common `error` strings:

| error | Trigger |
|---|---|
| `Invalid JSON` | invalid JSON (`type:"error"` envelope) |
| `Invalid request: expected a JSON object` | non-object payload (`type:"error"`) |
| `Missing 'type' field` | missing type (`type:"error"`) |
| `Missing 'callsign' for setter operation` | setter without callsign |
| `Flight plan not found: <callsign>` | flight plan missing |
| `Radar target not found: <callsign>` | radar target missing |
| `Voice channel not found: <name>` | channel missing |
| `Missing 'target' controller` | handoff/strip missing target |
| `Missing 'point' or 'time'` | set_estimation missing params |
| `Missing 'fp_callsign' or 'rt_callsign'` | correlate missing params |
| `Missing or invalid 'index' (0-8 required)` | invalid strip annotation index |
| `Missing or invalid 'value' for set_communication_type` | empty value |
| `Failed to set <field>` | setter/action failed |
| `Server busy: too many in-flight requests` | concurrency cap reached |
| `Unknown message type: <type>` | unsupported request type |

## Configuration constants

| Constant | Value |
|---|---|
| Port | `48521` (127.0.0.1 only) |
| Max inbound message size | `1 MB` (over-limit frames closed with `message_too_big`) |
| Max concurrent clients | `64` |
| Max concurrent requests | `64` (over-limit returns `Server busy`) |
| Per-client send queue | `32 MB` backpressure (slow consumers drop frames) |
| `timer` interval | `1 s` |

## Client implementation notes

- **Processing model**: each uplink request runs on its own worker
  thread; responses arrive as soon as they are ready; the IO thread is
  never blocked. Correlate responses by `id` (a connection may have
  several requests in flight).
- **Known gaps** (implementation differs from the wiki): `clear_estimation`
  is not implemented (returns `Unknown message type: clear_estimation`);
  `send_command` only echoes `{"command": ...}` and does not execute a
  command.
- **Subscription replies** use `"type": "response"` — do not expect a
  dedicated `subscription` type.
- **Robustness**: the server tolerates invalid UTF-8 (replaces bad
  bytes) and skips bad records in batches; clients should still handle
  timeouts and occasional `Server busy` responses.
