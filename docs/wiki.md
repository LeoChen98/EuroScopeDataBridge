# EuroScope Data Bridge — API Documentation

**Language: English | [中文](./wiki_CN.md)**

## Table of contents

- [Overview](#overview)
- [Connection](#connection)
- [Message format](#message-format)
  - [Request format](#request-format)
  - [Push event format](#push-event-format)
  - [Response format](#response-format)
- [Push events (server → client)](#push-events-server--client)
  - [radar_update](#radar_update)
  - [flightplan_update](#flightplan_update)
  - [flightplan_disconnect](#flightplan_disconnect)
  - [controller_update](#controller_update)
  - [controller_disconnect](#controller_disconnect)
  - [controller_assigned_data](#controller_assigned_data)
  - [flight_strip_pushed](#flight_strip_pushed)
  - [chat_private](#chat_private)
  - [chat_frequency](#chat_frequency)
  - [metar_received](#metar_received)
  - [plane_info](#plane_info)
  - [timer](#timer)
  - [airport_runway_activity_changed](#airport_runway_activity_changed)
- [Subscription](#subscription)
  - [subscribe](#subscribe)
  - [unsubscribe](#unsubscribe)
- [Requests (client → server)](#requests-client--server)
  - [Query](#query)
    - [get_flightplans](#get_flightplans)
    - [get_radar_targets](#get_radar_targets)
    - [get_controllers](#get_controllers)
    - [get_sector_elements](#get_sector_elements)
    - [get_voice_channels](#get_voice_channels)
    - [get_transition_altitude](#get_transition_altitude)
    - [get_connection_type](#get_connection_type)
    - [get_full_snapshot](#get_full_snapshot)
  - [Setters — ControllerAssignedData](#setters--controllerassigneddata)
    - [set_squawk](#set_squawk)
    - [set_final_altitude](#set_final_altitude)
    - [set_cleared_altitude](#set_cleared_altitude)
    - [set_scratchpad](#set_scratchpad)
    - [set_speed](#set_speed)
    - [set_mach](#set_mach)
    - [set_rate](#set_rate)
    - [set_heading](#set_heading)
    - [set_direct_to](#set_direct_to)
    - [set_communication_type](#set_communication_type)
  - [Setters — FlightPlanData](#setters--flightplandata)
    - [set_plan_type](#set_plan_type)
    - [set_aircraft_info](#set_aircraft_info)
    - [set_origin](#set_origin)
    - [set_destination](#set_destination)
    - [set_alternate](#set_alternate)
    - [set_remarks](#set_remarks)
    - [set_route](#set_route)
    - [set_true_airspeed](#set_true_airspeed)
    - [set_departure_time](#set_departure_time)
    - [set_actual_departure_time](#set_actual_departure_time)
    - [set_enroute_hours](#set_enroute_hours)
    - [set_enroute_minutes](#set_enroute_minutes)
    - [set_fuel_hours](#set_fuel_hours)
    - [set_fuel_minutes](#set_fuel_minutes)
  - [Actions](#actions)
    - [amend_flight_plan](#amend_flight_plan)
    - [start_tracking](#start_tracking)
    - [end_tracking](#end_tracking)
    - [accept_handoff](#accept_handoff)
    - [refuse_handoff](#refuse_handoff)
    - [initiate_handoff](#initiate_handoff)
    - [push_flight_strip](#push_flight_strip)
    - [set_estimation](#set_estimation)
    - [clear_estimation](#clear_estimation)
    - [set_flight_strip_annotation](#set_flight_strip_annotation)
    - [correlate](#correlate)
    - [uncorrelate](#uncorrelate)
    - [set_asel](#set_asel)
    - [display_message](#display_message)
    - [send_command](#send_command)
  - [Voice channel toggles](#voice-channel-toggles)
    - [toggle_primary](#toggle_primary)
    - [toggle_atis](#toggle_atis)
    - [toggle_text_receive](#toggle_text_receive)
    - [toggle_text_transmit](#toggle_text_transmit)
    - [toggle_voice_receive](#toggle_voice_receive)
    - [toggle_voice_transmit](#toggle_voice_transmit)
- [Data structure reference](#data-structure-reference)
  - [FlightPlan](#flightplan)
  - [FlightPlanData](#flightplandata)
  - [ControllerAssignedData](#controllerassigneddata)
  - [ExtractedRoute](#extractedroute)
  - [PositionPredictions](#positionpredictions)
  - [RadarTargetPositionData](#radartargetpositiondata)
  - [RadarTarget](#radartarget)
  - [Controller](#controller)
  - [SectorElement](#sectorelement)
  - [VoiceChannel](#voicechannel)
- [Error handling](#error-handling)
- [Configuration constants](#configuration-constants)

---

## Overview

EuroScope Data Bridge is an EuroScope plugin DLL that starts a WebSocket server on local `ws://127.0.0.1:48521`. All communication uses JSON.

Communication modes:

- **Push (subscription-based)**: EuroScope callback events are automatically serialized to JSON. A client must first send a `subscribe` request listing the event types it wants; after that, only subscribed clients receive the matching events. If no client has subscribed to an event type, the corresponding EuroScope callback is skipped entirely (no serialization, no push). See [Subscription](#subscription).
- **Request/Response**: the client sends a request with a unique `id`; the server returns a response carrying the same `id` after processing. Requests are processed inline as soon as they arrive, so the response is sent immediately.

Timing behavior:

- A `timer` event is sent every **1 second** — but only to clients that subscribed to `timer`.

---

## Connection

| Property | Value |
|----------|-------|
| Protocol | WebSocket (ws) |
| Host | `127.0.0.1` (local only) |
| Port | `48521` |

```javascript
const ws = new WebSocket('ws://127.0.0.1:48521');
```

---

## Message format

### Request format

All client → server requests use the following JSON structure:

```json
{
  "type": "<request type>",
  "id": "<client-generated unique ID>",
  "data": {
    // parameters (structure depends on type)
  }
}
```

| Field | Type | Required | Description |
|-------|------|----------|-------------|
| `type` | string | ✅ | Request type; see the request definitions below |
| `id` | string | Recommended | Client-generated unique ID, echoed back unchanged in the response (a random value is recommended) |
| `data` | object | No | Request parameters; structure depends on type |

### Push event format

Push events are only sent to clients that subscribed to their `type` (see [Subscription](#subscription)). Server-pushed events do not contain an `id` field:

```json
{
  "type": "<event type>",
  "data": {
    // event data
  }
}
```

| Field | Type | Description |
|-------|------|-------------|
| `type` | string | Event type |
| `data` | object | Event data |

### Response format

#### Success

```json
{
  "type": "response",
  "id": "<same id as the request>",
  "data": {
    "success": true,
    "result": { }
  }
}
```

| Field | Type | Description |
|-------|------|-------------|
| `type` | string | Always `"response"` |
| `id` | string | Same id as the request |
| `data.success` | bool | `true` |
| `data.result` | any | Return value of the operation (array or object for queries, `{}` for setters) |

#### Error

```json
{
  "type": "response",
  "id": "req-1",
  "data": {
    "success": false,
    "error": "error description"
  }
}
```

| Field | Type | Description |
|-------|------|-------------|
| `type` | string | Always `"response"` |
| `id` | string | Same id as the request |
| `data.success` | bool | `false` |
| `data.error` | string | Error description |

---

## Push events (server → client)

### radar_update

Radar target position update.

- **Trigger**: `OnRadarTargetPositionUpdate`
- **Frequency**: on every radar target position change

```json
{
  "type": "radar_update",
  "data": {
    // RadarTarget object, see [Data structure reference — RadarTarget](#radartarget)
  }
}
```

---

### flightplan_update

Flight plan data update.

- **Trigger**: `OnFlightPlanFlightPlanDataUpdate`
- **Frequency**: whenever flight plan data changes

```json
{
  "type": "flightplan_update",
  "data": {
    // FlightPlan object, see [Data structure reference — FlightPlan](#flightplan)
  }
}
```

---

### flightplan_disconnect

Flight plan disconnected (aircraft left the control area).

- **Trigger**: `OnFlightPlanDisconnect`

| Parameter | Type | Description |
|-----------|------|-------------|
| `data.callsign` | string | Callsign of the disconnected flight plan |

```json
{
  "type": "flightplan_disconnect",
  "data": {
    "callsign": "CES1234"
  }
}
```

---

### controller_update

Controller came online or changed position/status.

- **Trigger**: `OnControllerPositionUpdate`

```json
{
  "type": "controller_update",
  "data": {
    // Controller object, see [Data structure reference — Controller](#controller)
  }
}
```

---

### controller_disconnect

Controller disconnected.

- **Trigger**: `OnControllerDisconnect`

| Parameter | Type | Description |
|-----------|------|-------------|
| `data.callsign` | string | Controller callsign |
| `data.position_id` | string | Position ID |

```json
{
  "type": "controller_disconnect",
  "data": {
    "callsign": "ZBAA_APP",
    "position_id": "AAAP"
  }
}
```

If the controller is already invalid, `data` is an empty object `{}`.

---

### controller_assigned_data

The controller modified a flight plan's assigned data (squawk, altitude, speed, etc.).

- **Trigger**: `OnFlightPlanControllerAssignedDataUpdate`

```json
{
  "type": "controller_assigned_data",
  "data": {
    /* Complete FlightPlan object */
    "data_type": 1
  }
}
```

| Parameter | Type | Description |
|-----------|------|-------------|
| `data` | object | Complete FlightPlan object |
| `data.data_type` | int | Identifier of the changed data type (defined by EuroScope) |

---

### flight_strip_pushed

Flight strip pushed to another controller.

- **Trigger**: `OnFlightPlanFlightStripPushed`

| Parameter | Type | Description |
|-----------|------|-------------|
| `data.callsign` | string | Flight plan callsign |
| `data.sender` | string | Sender controller callsign |
| `data.target` | string | Receiving controller callsign |

```json
{
  "type": "flight_strip_pushed",
  "data": {
    "callsign": "CES1234",
    "sender": "ZBAA_APP",
    "target": "ZBAA_TWR"
  }
}
```

---

### chat_private

Private chat message.

- **Trigger**: `OnCompilePrivateChat`

| Parameter | Type | Description |
|-----------|------|-------------|
| `data.sender` | string | Sender callsign |
| `data.receiver` | string | Receiver callsign |
| `data.message` | string | Message content |

```json
{
  "type": "chat_private",
  "data": {
    "sender": "PK123",
    "receiver": "PK456",
    "message": "Hello, contact me on 121.5"
  }
}
```

---

### chat_frequency

Text chat message sent on a frequency.

- **Trigger**: `OnCompileFrequencyChat`

| Parameter | Type | Description |
|-----------|------|-------------|
| `data.sender` | string | Sender callsign |
| `data.frequency` | number | Frequency (MHz) |
| `data.message` | string | Message content |

```json
{
  "type": "chat_frequency",
  "data": {
    "sender": "PK123",
    "frequency": 121.5,
    "message": "CES1234, radar contact"
  }
}
```

---

### metar_received

New METAR weather report received.

- **Trigger**: `OnNewMetarReceived`

| Parameter | Type | Description |
|-----------|------|-------------|
| `data.station` | string | Weather station ICAO code (e.g. ZBAA) |
| `data.metar` | string | Full METAR report text |

```json
{
  "type": "metar_received",
  "data": {
    "station": "ZBAA",
    "metar": "ZBAA 010000Z 36006KT 320V040 CAVOK 25/M03 Q1017 NOSIG"
  }
}
```

---

### plane_info

Aircraft type/livery information update.

- **Trigger**: `OnPlaneInformationUpdate`

| Parameter | Type | Description |
|-----------|------|-------------|
| `data.callsign` | string | Aircraft callsign |
| `data.livery` | string | Livery/airline code |
| `data.plane_type` | string | Aircraft type code |

```json
{
  "type": "plane_info",
  "data": {
    "callsign": "CES1234",
    "livery": "CES",
    "plane_type": "A320"
  }
}
```

---

### timer

Timer event, triggered once per second.

- **Trigger**: `OnTimer`
- **Frequency**: 1 Hz

| Parameter | Type | Description |
|-----------|------|-------------|
| `data.counter` | int | Seconds elapsed since plugin start |

```json
{
  "type": "timer",
  "data": {
    "counter": 42
  }
}
```

---

### airport_runway_activity_changed

Runway activity status change.

- **Trigger**: `OnAirportRunwayActivityChanged`

```json
{
  "type": "airport_runway_activity_changed",
  "data": {}
}
```

---

## Subscription

Push events are **not** delivered by default. A client must subscribe to the event types it wants; only subscribed clients receive the matching events. When no client has subscribed to an event type, the corresponding EuroScope callback is skipped entirely (no serialization, no push).

- Subscriptions are **per client** (each WebSocket connection has its own set).
- Subscriptions are cleared automatically when a client disconnects.
- Unknown event type strings are accepted but never produce events; the full list of push event types is in [Push events (server → client)](#push-events-server--client).

### subscribe

Adds the given event types to the client's subscription set. Repeatable — calling it again with more types appends them.

**Request example:**

```json
{ "type": "subscribe", "id": "sub-1", "data": { "events": ["radar_update", "flightplan_update", "timer"] } }
```

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `data.events` | string[] | ✅ | Event types to subscribe to |

**Response example:**

```json
{
  "type": "response",
  "id": "sub-1",
  "data": {
    "success": true,
    "events": ["flightplan_update", "radar_update", "timer"]
  }
}
```

| Field | Type | Description |
|-------|------|-------------|
| `data.success` | bool | `true` |
| `data.events` | string[] | The client's full current subscription set after this operation |

### unsubscribe

Removes the given event types from the client's subscription set. If `data.events` is omitted or an empty array, **all** subscriptions are cleared.

**Request example:**

```json
{ "type": "unsubscribe", "id": "sub-2", "data": { "events": ["timer"] } }

// Clear all subscriptions
{ "type": "unsubscribe", "id": "sub-3" }
```

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `data.events` | string[] | No | Event types to unsubscribe; omit or pass `[]` to clear all |

The response has the same shape as `subscribe` (`success` + current `events`).

**Error example** (`data.events` is not an array):

```json
{
  "type": "response",
  "id": "sub-1",
  "data": {
    "success": false,
    "error": "Invalid 'events' field; expected an array of strings"
  }
}
```

---

## Requests (client → server)

### Query

#### get_flightplans

Retrieves flight plans. Without `callsign`, returns all; with it, returns only the matching one.

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `data.callsign` | string | No | Flight plan callsign; omit to return all |

**Request example:**

```json
// Query all
{ "type": "get_flightplans", "id": "1" }

// Query a single one
{ "type": "get_flightplans", "id": "1", "data": { "callsign": "CES1234" } }
```

**Response example:**

```json
{
  "type": "response",
  "id": "1",
  "data": {
    "success": true,
    "result": [
      // FlightPlan[]
    ]
  }
}
```

---

#### get_radar_targets

Retrieves radar targets. Without `callsign`, returns all.

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `data.callsign` | string | No | Radar target callsign; omit to return all |

**Request example:**

```json
{ "type": "get_radar_targets", "id": "2" }
{ "type": "get_radar_targets", "id": "2", "data": { "callsign": "CES1234" } }
```

**Response example:**

```json
{
  "type": "response",
  "id": "2",
  "data": {
    "success": true,
    "result": [
      // RadarTarget[]
    ]
  }
}
```

---

#### get_controllers

Retrieves all online controllers (includes yourself automatically).

- **No parameters**

**Request example:**

```json
{ "type": "get_controllers", "id": "3" }
```

**Response example:**

```json
{
  "type": "response",
  "id": "3",
  "data": {
    "success": true,
    "result": [
      // Controller[]
    ]
  }
}
```

---

#### get_sector_elements

Retrieves sector file elements.

| Parameter | Type | Required | Default | Description |
|-----------|------|----------|---------|-------------|
| `data.element_type` | int | No | `-1` | Element type filter; `-1` means all |

**Request example:**

```json
{ "type": "get_sector_elements", "id": "4", "data": { "element_type": -1 } }
```

**Response example:**

```json
{
  "type": "response",
  "id": "4",
  "data": {
    "success": true,
    "result": [
      // SectorElement[]
    ]
  }
}
```

---

#### get_voice_channels

Retrieves all voice channels.

- **No parameters**

**Request example:**

```json
{ "type": "get_voice_channels", "id": "5" }
```

**Response example:**

```json
{
  "type": "response",
  "id": "5",
  "data": {
    "success": true,
    "result": [
      // VoiceChannel[]
    ]
  }
}
```

---

#### get_transition_altitude

Retrieves the transition altitude of the current sector.

- **No parameters**

**Request example:**

```json
{ "type": "get_transition_altitude", "id": "6" }
```

**Response example:**

```json
{
  "type": "response",
  "id": "6",
  "data": {
    "success": true,
    "result": {
      "transition_altitude": 9800
    }
  }
}
```

| Response field | Type | Description |
|----------------|------|-------------|
| `result.transition_altitude` | int | Transition altitude (feet) |

---

#### get_connection_type

Retrieves the current connection type.

- **No parameters**

**Request example:**

```json
{ "type": "get_connection_type", "id": "7" }
```

**Response example:**

```json
{
  "type": "response",
  "id": "7",
  "data": {
    "success": true,
    "result": {
      "connection_type": 1
    }
  }
}
```

| Response field | Type | Description |
|----------------|------|-------------|
| `result.connection_type` | int | Connection type code |

---

#### get_full_snapshot

Retrieves a complete snapshot of the current state, including all flight plans, radar targets and online controllers.

- **No parameters**

**Request example:**

```json
{ "type": "get_full_snapshot", "id": "8" }
```

**Response example:**

```json
{
  "type": "response",
  "id": "8",
  "data": {
    "success": true,
    "result": {
      "flightplans": [
        // FlightPlan[], see data structure reference
      ],
      "radar_targets": [
        // RadarTarget[], see data structure reference
      ],
      "controllers": [
        // Controller[], see data structure reference
      ]
    }
  }
}
```

| Response field | Type | Description |
|----------------|------|-------------|
| `result.flightplans` | array | All FlightPlan objects |
| `result.radar_targets` | array | All RadarTarget objects |
| `result.controllers` | array | All Controller objects (including yourself) |

---

### Setters — ControllerAssignedData

The following requests operate on a flight plan's **ControllerAssignedData** (temporary data assigned by the controller, overriding the flight plan's original values).

**Common parameters:** all requests in this category require `data.callsign` and the target value (passed via `data.value`, either string or integer).

#### set_squawk

Sets the squawk code.

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `data.callsign` | string | ✅ | Flight plan callsign |
| `data.value` | string | ✅ | Squawk code (4-digit string) |

```json
{ "type": "set_squawk", "id": "r1", "data": { "callsign": "CES1234", "value": "5678" } }
```

---

#### set_final_altitude

Sets the controller-assigned final altitude (overrides the flight plan's original final altitude).

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `data.callsign` | string | ✅ | Flight plan callsign |
| `data.value` | int | ✅ | Final altitude (feet) |

```json
{ "type": "set_final_altitude", "id": "r2", "data": { "callsign": "CES1234", "value": 35000 } }
```

---

#### set_cleared_altitude

Sets the cleared altitude.

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `data.callsign` | string | ✅ | Flight plan callsign |
| `data.value` | int | ✅ | Cleared altitude (feet) |

```json
{ "type": "set_cleared_altitude", "id": "r3", "data": { "callsign": "CES1234", "value": 5000 } }
```

---

#### set_scratchpad

Sets the scratchpad text.

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `data.callsign` | string | ✅ | Flight plan callsign |
| `data.value` | string | ✅ | Scratchpad text |

```json
{ "type": "set_scratchpad", "id": "r4", "data": { "callsign": "CES1234", "value": "ANYTHING" } }
```

---

#### set_speed

Sets the assigned speed (IAS, knots).

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `data.callsign` | string | ✅ | Flight plan callsign |
| `data.value` | int | ✅ | Speed (knots) |

```json
{ "type": "set_speed", "id": "r5", "data": { "callsign": "CES1234", "value": 250 } }
```

---

#### set_mach

Sets the assigned Mach number.

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `data.callsign` | string | ✅ | Flight plan callsign |
| `data.value` | int | ✅ | Mach number × 100 (e.g. 78 means M.78) |

```json
{ "type": "set_mach", "id": "r6", "data": { "callsign": "CES1234", "value": 78 } }
```

---

#### set_rate

Sets the rate of climb/descent (feet per minute).

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `data.callsign` | string | ✅ | Flight plan callsign |
| `data.value` | int | ✅ | Rate (ft/min) |

```json
{ "type": "set_rate", "id": "r7", "data": { "callsign": "CES1234", "value": 1500 } }
```

---

#### set_heading

Sets the assigned heading (degrees).

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `data.callsign` | string | ✅ | Flight plan callsign |
| `data.value` | int | ✅ | Heading (0-360) |

```json
{ "type": "set_heading", "id": "r8", "data": { "callsign": "CES1234", "value": 180 } }
```

---

#### set_direct_to

Sets a direct-to waypoint.

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `data.callsign` | string | ✅ | Flight plan callsign |
| `data.value` | string | ✅ | Waypoint name |

```json
{ "type": "set_direct_to", "id": "r9", "data": { "callsign": "CES1234", "value": "LADIX" } }
```

---

#### set_communication_type

Sets the communication type.

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `data.callsign` | string | ✅ | Flight plan callsign |
| `data.value` | string | ✅ | Single-character communication type code (`V` = voice, `T` = text, `R` = voice receive only) |

```json
{ "type": "set_communication_type", "id": "r10", "data": { "callsign": "CES1234", "value": "V" } }
```

---

### Setters — FlightPlanData

The following requests modify a flight plan's **FlightPlanData** (raw flight plan data fields).

**Common parameters:** all requests in this category require `data.callsign` and the target value (passed via `data.value`).

#### set_plan_type

Sets the flight plan type.

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `data.callsign` | string | ✅ | Flight plan callsign |
| `data.value` | string | ✅ | Plan type (e.g. `"I"`, `"V"`) |

```json
{ "type": "set_plan_type", "id": "r11", "data": { "callsign": "CES1234", "value": "I" } }
```

---

#### set_aircraft_info

Sets the aircraft information (ICAO aircraft type string).

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `data.callsign` | string | ✅ | Flight plan callsign |
| `data.value` | string | ✅ | ICAO aircraft type string |

```json
{ "type": "set_aircraft_info", "id": "r12", "data": { "callsign": "CES1234", "value": "A320/M-SDE2E3FGHIRWXY/LB1" } }
```

---

#### set_origin

Sets the origin airport.

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `data.callsign` | string | ✅ | Flight plan callsign |
| `data.value` | string | ✅ | ICAO code |

```json
{ "type": "set_origin", "id": "r13", "data": { "callsign": "CES1234", "value": "ZBAA" } }
```

---

#### set_destination

Sets the destination airport.

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `data.callsign` | string | ✅ | Flight plan callsign |
| `data.value` | string | ✅ | ICAO code |

```json
{ "type": "set_destination", "id": "r14", "data": { "callsign": "CES1234", "value": "ZSPD" } }
```

---

#### set_alternate

Sets the alternate airport.

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `data.callsign` | string | ✅ | Flight plan callsign |
| `data.value` | string | ✅ | ICAO code |

```json
{ "type": "set_alternate", "id": "r15", "data": { "callsign": "CES1234", "value": "ZSNJ" } }
```

---

#### set_remarks

Sets the remarks (RMK/).

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `data.callsign` | string | ✅ | Flight plan callsign |
| `data.value` | string | ✅ | Remarks text |

```json
{ "type": "set_remarks", "id": "r16", "data": { "callsign": "CES1234", "value": "RMK/TCAS EQUIPPED" } }
```

---

#### set_route

Sets the route string.

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `data.callsign` | string | ✅ | Flight plan callsign |
| `data.value` | string | ✅ | Full route string |

```json
{ "type": "set_route", "id": "r17", "data": { "callsign": "CES1234", "value": "LADIX W40 YQG W34 ANRAT" } }
```

---

#### set_true_airspeed

Sets the true airspeed (knots).

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `data.callsign` | string | ✅ | Flight plan callsign |
| `data.value` | int | ✅ | True airspeed (knots) |

```json
{ "type": "set_true_airspeed", "id": "r18", "data": { "callsign": "CES1234", "value": 460 } }
```

---

#### set_departure_time

Sets the estimated off-block time (EOBT).

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `data.callsign` | string | ✅ | Flight plan callsign |
| `data.value` | string | ✅ | Time string (format depends on EuroScope) |

```json
{ "type": "set_departure_time", "id": "r19", "data": { "callsign": "CES1234", "value": "1200" } }
```

---

#### set_actual_departure_time

Sets the actual departure time (ATD).

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `data.callsign` | string | ✅ | Flight plan callsign |
| `data.value` | string | ✅ | Time string |

```json
{ "type": "set_actual_departure_time", "id": "r20", "data": { "callsign": "CES1234", "value": "1215" } }
```

---

#### set_enroute_hours

Sets the enroute hours.

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `data.callsign` | string | ✅ | Flight plan callsign |
| `data.value` | string | ✅ | Hours string |

```json
{ "type": "set_enroute_hours", "id": "r21", "data": { "callsign": "CES1234", "value": "1" } }
```

---

#### set_enroute_minutes

Sets the enroute minutes.

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `data.callsign` | string | ✅ | Flight plan callsign |
| `data.value` | string | ✅ | Minutes string |

```json
{ "type": "set_enroute_minutes", "id": "r22", "data": { "callsign": "CES1234", "value": "30" } }
```

---

#### set_fuel_hours

Sets the fuel hours.

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `data.callsign` | string | ✅ | Flight plan callsign |
| `data.value` | string | ✅ | Hours string |

```json
{ "type": "set_fuel_hours", "id": "r23", "data": { "callsign": "CES1234", "value": "2" } }
```

---

#### set_fuel_minutes

Sets the fuel minutes.

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `data.callsign` | string | ✅ | Flight plan callsign |
| `data.value` | string | ✅ | Minutes string |

```json
{ "type": "set_fuel_minutes", "id": "r24", "data": { "callsign": "CES1234", "value": "0" } }
```

---

### Actions

#### amend_flight_plan

Submits flight plan amendments (makes FlightPlanData changes take effect).

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `data.callsign` | string | ✅ | Flight plan callsign |

```json
{ "type": "amend_flight_plan", "id": "a1", "data": { "callsign": "CES1234" } }
```

---

#### start_tracking

Starts tracking a flight plan.

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `data.callsign` | string | ✅ | Flight plan callsign |

```json
{ "type": "start_tracking", "id": "a2", "data": { "callsign": "CES1234" } }
```

---

#### end_tracking

Stops tracking a flight plan.

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `data.callsign` | string | ✅ | Flight plan callsign |

```json
{ "type": "end_tracking", "id": "a3", "data": { "callsign": "CES1234" } }
```

---

#### accept_handoff

Accepts a handoff.

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `data.callsign` | string | ✅ | Flight plan callsign |

```json
{ "type": "accept_handoff", "id": "a4", "data": { "callsign": "CES1234" } }
```

---

#### refuse_handoff

Refuses a handoff.

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `data.callsign` | string | ✅ | Flight plan callsign |

```json
{ "type": "refuse_handoff", "id": "a5", "data": { "callsign": "CES1234" } }
```

---

#### initiate_handoff

Initiates a handoff to the specified controller.

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `data.callsign` | string | ✅ | Flight plan callsign |
| `data.target` | string | ✅ | Target controller callsign |

```json
{ "type": "initiate_handoff", "id": "a6", "data": { "callsign": "CES1234", "target": "ZBAA_TWR" } }
```

---

#### push_flight_strip

Pushes the flight strip to the specified controller.

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `data.callsign` | string | ✅ | Flight plan callsign |
| `data.target` | string | ✅ | Target controller callsign |

```json
{ "type": "push_flight_strip", "id": "a7", "data": { "callsign": "CES1234", "target": "ZBAA_TWR" } }
```

---

#### set_estimation

Sets the estimated time over a point.

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `data.callsign` | string | ✅ | Flight plan callsign |
| `data.point` | string | ✅ | Waypoint name |
| `data.time` | string | ✅ | Estimated time string |

```json
{ "type": "set_estimation", "id": "a8", "data": { "callsign": "CES1234", "point": "LADIX", "time": "1230" } }
```

---

#### clear_estimation

Clears estimated times. Without `point`, clears all estimated times; with `point`, clears only the estimated time of the specified waypoint.

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `data.callsign` | string | ✅ | Flight plan callsign |
| `data.point` | string | No | Waypoint name; omit to clear all |

```json
// Clear all
{ "type": "clear_estimation", "id": "a9", "data": { "callsign": "CES1234" } }

// Clear a specific point
{ "type": "clear_estimation", "id": "a9", "data": { "callsign": "CES1234", "point": "LADIX" } }
```

---

#### set_flight_strip_annotation

Sets a flight strip annotation (rows 0-8).

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `data.callsign` | string | ✅ | Flight plan callsign |
| `data.index` | int | ✅ | Annotation row index (0-8) |
| `data.annotation` | string | ✅ | Annotation text |

```json
{ "type": "set_flight_strip_annotation", "id": "a10", "data": { "callsign": "CES1234", "index": 0, "annotation": "PEL" } }
```

---

#### correlate

Correlates a flight plan with a radar target.

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `data.fp_callsign` | string | ✅ | Flight plan callsign |
| `data.rt_callsign` | string | ✅ | Radar target callsign |

```json
{ "type": "correlate", "id": "a11", "data": { "fp_callsign": "CES1234", "rt_callsign": "CES1234" } }
```

---

#### uncorrelate

Uncorrelates a flight plan from a radar target.

| Parameter | Type | Required | Default | Description |
|-----------|------|----------|---------|-------------|
| `data.callsign` | string | ✅ | — | Flight plan or radar target callsign |
| `data.target_type` | string | No | `"fp"` | Target type: `"fp"` or `"rt"` / `"radar_target"` |

```json
// Uncorrelate from the flight plan side
{ "type": "uncorrelate", "id": "a12", "data": { "callsign": "CES1234", "target_type": "fp" } }

// Uncorrelate from the radar target side
{ "type": "uncorrelate", "id": "a12", "data": { "callsign": "CES1234", "target_type": "rt" } }
```

---

#### set_asel

Sets the currently selected aircraft (ASEL).

| Parameter | Type | Required | Default | Description |
|-----------|------|----------|---------|-------------|
| `data.callsign` | string | ✅ | — | Flight plan or radar target callsign |
| `data.asel_type` | string | No | `"fp"` | Target type: `"fp"` or `"rt"` / `"radar_target"` |

```json
{ "type": "set_asel", "id": "a13", "data": { "callsign": "CES1234", "asel_type": "fp" } }
```

---

#### display_message

Displays a message in the EuroScope UI.

| Parameter | Type | Required | Default | Description |
|-----------|------|----------|---------|-------------|
| `data.handler` | string | No | `"DataBridge"` | Message handler name |
| `data.sender` | string | No | — | Sender name |
| `data.message` | string | No | — | Message content |
| `data.show` | bool | No | `true` | Whether to show the handler name |
| `data.unread` | bool | No | `true` | Whether to mark as unread |
| `data.busy` | bool | No | `true` | Whether to still display when busy |
| `data.flash` | bool | No | `true` | Whether to flash |
| `data.confirm` | bool | No | `false` | Whether user confirmation is required |

```json
{
  "type": "display_message",
  "id": "a14",
  "data": {
    "sender": "External system",
    "message": "Data sync complete",
    "flash": true,
    "confirm": false
  }
}
```

---

#### send_command

Sends a raw command to EuroScope.

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `data.command` | string | ✅ | EuroScope command string |

```json
{ "type": "send_command", "id": "a15", "data": { "command": ".msg PK123 Hello" } }
```

---

### Voice channel toggles

All toggle operations share the same parameter structure. The `channel` parameter specifies the target channel name; if omitted, the first available channel is used.

#### toggle_primary

Toggles the primary channel.

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `data.channel` | string | No | Channel name; omit to use the first available channel |

```json
{ "type": "toggle_primary", "id": "v1", "data": { "channel": "ZBAA_APP" } }
```

---

#### toggle_atis

Toggles the ATIS channel.

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `data.channel` | string | No | Channel name |

```json
{ "type": "toggle_atis", "id": "v2", "data": { "channel": "ZBAA_ATIS" } }
```

---

#### toggle_text_receive

Toggles text reception.

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `data.channel` | string | No | Channel name |

```json
{ "type": "toggle_text_receive", "id": "v3", "data": { "channel": "ZBAA_APP" } }
```

---

#### toggle_text_transmit

Toggles text transmission.

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `data.channel` | string | No | Channel name |

```json
{ "type": "toggle_text_transmit", "id": "v4", "data": { "channel": "ZBAA_APP" } }
```

---

#### toggle_voice_receive

Toggles voice reception.

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `data.channel` | string | No | Channel name |

```json
{ "type": "toggle_voice_receive", "id": "v5", "data": { "channel": "ZBAA_APP" } }
```

---

#### toggle_voice_transmit

Toggles voice transmission.

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `data.channel` | string | No | Channel name |

```json
{ "type": "toggle_voice_transmit", "id": "v6", "data": { "channel": "ZBAA_APP" } }
```

---

## Data structure reference

### FlightPlan

Complete flight plan object. Appears in `flightplan_update`, `controller_assigned_data`, `full_snapshot` and `get_flightplans`.

```jsonc
{
  "valid": true,                        // bool   — whether the object is valid
  "callsign": "CES1234",                // string — callsign
  "pilot_name": "Leo Chen",             // string — pilot name
  "state": 2,                           // int    — state code
  "fp_state": 0,                        // int    — flight plan state
  "simulated": false,                   // bool   — whether it is a simulator

  // --- Tracking ---
  "tracking_controller_callsign": "ZBAA_APP",  // string — tracking controller callsign
  "tracking_controller_id": "AAAA",           // string — tracking controller ID
  "tracking_controller_is_me": true,           // bool   — whether I am tracking

  // --- Handoff ---
  "handoff_target_callsign": "ZBAA_TWR",       // string — handoff target controller callsign
  "handoff_target_id": "AAAT",                // string — handoff target controller ID

  // --- Route ---
  "distance_to_destination": 450.3,     // double — distance to destination (NM)
  "distance_from_origin": 23.1,        // double — distance from origin (NM)
  "next_copx": "LADIX",                // string — next COPX point
  "next_fir_copx": "",                 // string — next FIR boundary point
  "sector_entry_minutes": 5,           // int    — minutes to sector entry
  "sector_exit_minutes": 12,           // int    — minutes to sector exit

  // --- Flags ---
  "ram_flag": true,                    // bool   — RAM flag
  "clam_flag": false,                  // bool   — CLAM flag
  "ground_state": "AIR",               // string — ground state (AIR/GND/...)
  "clearence_flag": 1,                 // int    — clearance flag
  "text_communication": false,         // bool   — whether using text communication

  // --- Altitudes ---
  "final_altitude": 35000,              // int    — final altitude (ft)
  "cleared_altitude": 5000,            // int    — cleared altitude (ft)

  // --- Coordination (entry) ---
  "entry_coordination_point_name": "LADIX",     // string — entry coordination point
  "entry_coordination_point_state": 1,          // int    — entry coordination point state
  "entry_coordination_altitude_state": 1,       // int    — entry coordination altitude state
  "entry_coordination_altitude": 35000,          // int    — entry coordination altitude

  // --- Coordination (exit) ---
  "exit_coordination_point_name": "ANRAT",      // string — exit coordination point
  "exit_coordination_name_state": 1,            // int    — exit coordination point name state
  "exit_coordination_altitude_state": 1,        // int    — exit coordination altitude state
  "exit_coordination_altitude": 35000,           // int    — exit coordination altitude

  // --- Next controller ---
  "coordinated_next_controller": "ZBAA_TWR",    // string — coordinated next controller
  "coordinated_next_controller_state": 1,       // int    — coordination state

  // --- Sub-objects ---
  "flight_plan_data":             { /* FlightPlanData */ },
  "controller_assigned_data":     { /* ControllerAssignedData */ },
  "extracted_route":              { /* ExtractedRoute */ },
  "position_predictions":         { /* PositionPredictions */ },
  "track_position":               { /* RadarTargetPositionData */ }
}
```

---

### FlightPlanData

Raw flight plan data.

```jsonc
{
  "is_received": true,                // bool   — whether received
  "is_amended": false,                // bool   — whether amended
  "plan_type": "I",                   // string — plan type (I/V)
  "aircraft_info": "A320/M-SDE2E3FGHIRWXY/LB1",  // string — ICAO aircraft type string
  "aircraft_wtc": "M",                // char   — wake turbulence category (L/M/H/J)
  "aircraft_type": "J",               // char   — aircraft type
  "engine_number": 2,                 // int    — number of engines
  "engine_type": "J",                 // char   — engine type
  "capabilities": "S",                // char   — equipment capability code
  "is_rvsm": true,                    // bool   — RVSM capability
  "manufacturer_type": "A320",        // string — manufacturer type
  "aircraft_fp_type": "A320",         // string — aircraft type in flight plan
  "true_airspeed": 460,               // int    — true airspeed (kt)
  "origin": "ZBAA",                   // string — origin airport ICAO
  "destination": "ZSPD",              // string — destination airport ICAO
  "alternate": "ZSNJ",                // string — alternate airport ICAO
  "final_altitude": 35000,             // int    — final altitude (ft)
  "remarks": "",                      // string — remarks (RMK/)
  "communication_type": "V",          // char   — communication type
  "route": "LADIX W40 YQG W34 ANRAT", // string — route
  "sid_name": "LADIX11D",             // string — SID name
  "star_name": "",                    // string — STAR name
  "departure_rwy": "36R",             // string — departure runway
  "arrival_rwy": "",                  // string — arrival runway
  "estimated_departure_time": "1200", // string — EOBT
  "actual_departure_time": "",        // string — ATD
  "enroute_hours": "1",               // string — enroute hours
  "enroute_minutes": "30",            // string — enroute minutes
  "fuel_hours": "2",                  // string — fuel hours
  "fuel_minutes": "0"                 // string — fuel minutes
}
```

---

### ControllerAssignedData

Temporary data assigned by the controller.

```jsonc
{
  "squawk": "1234",                   // string — squawk code
  "final_altitude": 9800,             // int    — controller-assigned final altitude
  "cleared_altitude": 5000,           // int    — cleared altitude
  "communication_type": "V",          // char   — communication type
  "scratchpad": "",                   // string — scratchpad
  "assigned_speed": 250,              // int    — assigned speed (kt)
  "assigned_mach": 0,                 // int    — assigned Mach ×100
  "assigned_rate": 0,                 // int    — assigned rate (ft/min)
  "assigned_heading": 180,            // int    — assigned heading (°)
  "direct_to": "",                    // string — direct-to point
  "flight_strip_annotations": [       // string[] — flight strip annotations (indices 0-8)
    "", "", "", "", "", "", "", "", ""
  ]
}
```

---

### ExtractedRoute

Extracted route data.

```jsonc
{
  "points_number": 4,                 // int  — number of waypoints
  "calculated_index": 0,              // int  — calculated index
  "assigned_index": 0,                // int  — assigned index
  "points": [
    {
      "index": 0,                     // int    — index
      "name": "LADIX",                // string — waypoint name
      "latitude": 40.123,             // double — latitude
      "longitude": 116.456,           // double — longitude
      "airway_name": "W40",           // string — airway name
      "airway_classification": 1,     // int    — airway classification
      "distance_in_minutes": 0,       // int    — distance (minutes)
      "calculated_profile_altitude": 35000  // int — calculated profile altitude
    }
  ]
}
```

---

### PositionPredictions

Position prediction data.

```jsonc
{
  "points_number": 3,                 // int  — number of prediction points
  "points": [
    {
      "index": 0,                     // int    — index
      "latitude": 39.5,               // double — latitude
      "longitude": 117.1,             // double — longitude
      "altitude": 35000,               // int    — altitude (ft)
      "controller_id": "ZBAA_APP"     // string — controller ID
    }
  ]
}
```

---

### RadarTargetPositionData

Radar target position data (single position snapshot).

```jsonc
{
  "valid": true,                      // bool   — whether valid
  "latitude": 39.923,                 // double — latitude
  "longitude": 116.587,               // double — longitude
  "flight_level": 350,                // int    — altitude (FL)
  "squawk": "1234",                   // string — transponder
  "reported_gs": 460,                 // int    — ground speed (kt)
  "reported_heading": 180,            // int    — magnetic heading (°)
  "reported_heading_true_north": 176, // int    — true heading (°)
  "reported_pitch": 0,                // int    — pitch
  "reported_bank": 0,                 // int    — bank
  "radar_flags": 0,                   // int    — radar flags
  "received_time": 1234567890,        // int    — received timestamp
  "transponder_c": true,              // bool   — Mode C transponder
  "transponder_i": false              // bool   — Mode I transponder
}
```

---

### RadarTarget

Complete radar target object. Appears in `radar_update`, `full_snapshot` and `get_radar_targets`.

```jsonc
{
  "valid": true,                      // bool   — whether valid
  "callsign": "CES1234",              // string — callsign
  "system_id": "RADAR1",              // string — radar station ID
  "vertical_speed": 1500,             // int    — vertical speed (ft/min)
  "track_heading": 178,               // int    — track heading (°)
  "ground_speed": 460,                // int    — ground speed (kt)
  "correlated_callsign": "CES1234",   // string — correlated flight plan callsign (null if uncorrelated)
  "position": {
    // RadarTargetPositionData — current position
  },
  "position_history": [
    // RadarTargetPositionData[] — position history (newest first)
  ]
}
```

---

### Controller

Controller object. Appears in `controller_update`, `full_snapshot` and `get_controllers`.

```jsonc
{
  "valid": true,                      // bool   — whether valid
  "callsign": "ZBAA_APP",                // string — controller callsign
  "position_id": "AAAA",          // string — position ID
  "identified": true,                 // bool   — whether identified
  "primary_frequency": 121.100,       // double — primary frequency (MHz)
  "full_name": "Leo Chen",            // string — full name
  "rating": 5,                        // int    — controller rating
  "facility": 1,                      // int    — facility type
  "sector_file": "ZBAA.sct",          // string — sector file name
  "is_controller": true,              // bool   — whether it is a controller
  "position": {
    "latitude": 40.08,                // double — latitude
    "longitude": 116.58               // double — longitude
  },
  "range": 200,                       // int    — visible range (NM)
  "is_breaking": false,               // bool   — whether disconnecting
  "ongoing_able": true                // bool   — whether handoff can be continued
}
```

---

### SectorElement

Sector element object. Appears in `get_sector_elements`.

```jsonc
{
  "valid": true,                      // bool   — whether valid
  "name": "ZBAA_APP",                 // string — element name
  "element_type": 1,                  // int    — element type
  "positions": [
    { "latitude": 39.5, "longitude": 116.0 },   // polygon vertices
    { "latitude": 40.5, "longitude": 117.0 }
  ],
  "components": ["ZBAA_APP"],         // string[] — component names
  "frequency": 121.1,                 // double — frequency (MHz)
  "runways": [
    { "name": "36R", "heading": 359 },  // runway
    { "name": "18L", "heading": 179 }
  ],
  "airport_name": "ZBAA",             // string — airport name
  "active_arrival": true,             // bool   — active arrival
  "active_departure": false           // bool   — active departure
}
```

---

### VoiceChannel

Voice channel object. Appears in `get_voice_channels`.

```jsonc
{
  "valid": true,                      // bool   — whether valid
  "name": "ZBAA_APP",                 // string — channel name
  "frequency": 121.100,               // double — frequency (MHz)
  "voice_server": "voice.example.com",// string — voice server address
  "voice_channel": "ZBAA_APP",        // string — voice channel ID
  "is_primary": true,                 // bool   — whether primary
  "is_atis": false,                   // bool   — whether ATIS
  "is_text_receive_on": true,         // bool   — whether text receive is on
  "is_text_transmit_on": false,       // bool   — whether text transmit is on
  "is_voice_receive_on": true,        // bool   — whether voice receive is on
  "is_voice_transmit_on": true,       // bool   — whether voice transmit is on
  "is_voice_connected": true          // bool   — whether voice is connected
}
```

---

## Error handling

All errors are returned via `response` messages with `data.success` set to `false`.

### Common error messages

| Error message | Trigger condition |
|---------------|-------------------|
| `Missing 'type' field` | The request is missing the `type` field |
| `Invalid request: expected a JSON object` | The request payload is not a JSON object |
| `Missing 'callsign' for setter operation` | A setter operation is missing `data.callsign` |
| `Flight plan not found: <callsign>` | The specified flight plan does not exist |
| `Radar target not found: <callsign>` | The specified radar target does not exist |
| `Voice channel not found: <name>` | The specified voice channel was not found |
| `Missing 'target' controller` | `initiate_handoff` or `push_flight_strip` is missing `target` |
| `Missing 'point' or 'time'` | `set_estimation` is missing `point` or `time` |
| `Missing 'fp_callsign' or 'rt_callsign'` | `correlate` is missing parameters |
| `Missing or invalid 'index' (0-8 required)` | `set_flight_strip_annotation` has an invalid `index` |
| `Missing or invalid 'value' for set_communication_type` | `set_communication_type` has an empty or missing `value` |
| `Failed to set <field>` | A setter returned an error (target invalid or value rejected) |
| `Unknown message type: <type>` | Unsupported request type |
| `Invalid JSON` | The request is not valid JSON (returned immediately, not queued for processing) |

---

## Configuration constants

| Constant | Value | Description |
|----------|-------|-------------|
| WebSocket port | `48521` | Listens on `127.0.0.1` only |
| Max inbound message size | `1 MB` | Larger frames are rejected with the `message_too_big` protocol error |
| Timer interval | `1 s` | `timer` event frequency |

