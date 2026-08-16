# EuroScope Data Bridge — API 文档

**语言：[English](./wiki.md) | 中文**

## 目录

- [概述](#概述)
- [连接](#连接)
- [消息格式](#消息格式)
  - [请求格式](#请求格式)
  - [Push 事件格式](#push-事件格式)
  - [响应格式](#响应格式)
- [Push 事件（服务端 → 客户端）](#push-事件服务端--客户端)
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
- [订阅](#订阅)
  - [subscribe](#subscribe)
  - [unsubscribe](#unsubscribe)
- [心跳（Ping / Pong）](#心跳ping--pong)
- [请求（客户端 → 服务端）](#请求客户端--服务端)
  - [查询类](#查询类)
    - [get_flightplans](#get_flightplans)
    - [get_radar_targets](#get_radar_targets)
    - [get_controllers](#get_controllers)
    - [get_sector_elements](#get_sector_elements)
    - [get_voice_channels](#get_voice_channels)
    - [get_transition_altitude](#get_transition_altitude)
    - [get_connection_type](#get_connection_type)
    - [get_full_snapshot](#get_full_snapshot)
  - [设置类 — ControllerAssignedData](#设置类--controllerassigneddata)
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
  - [设置类 — FlightPlanData](#设置类--flightplandata)
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
  - [操作类](#操作类)
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
  - [语音频道切换](#语音频道切换)
    - [toggle_primary](#toggle_primary)
    - [toggle_atis](#toggle_atis)
    - [toggle_text_receive](#toggle_text_receive)
    - [toggle_text_transmit](#toggle_text_transmit)
    - [toggle_voice_receive](#toggle_voice_receive)
    - [toggle_voice_transmit](#toggle_voice_transmit)
- [数据结构参考](#数据结构参考)
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
- [错误处理](#错误处理)
- [配置常量](#配置常量)

---

## 概述

EuroScope Data Bridge 是一个 EuroScope 插件 DLL，在本地 `ws://127.0.0.1:48521` 启动 WebSocket 服务。所有通信均使用 JSON 格式。

通信模式：

- **Push（订阅推送）**：EuroScope 回调事件自动序列化为 JSON。客户端需先用 `subscribe` 请求订阅感兴趣的事件类型，此后仅订阅的客户端会收到对应事件；没有任何客户端订阅某事件时，该事件对应的回调会被跳过（不做序列化与推送）。详见 [订阅](#订阅)。
- **Request/Response（请求/响应）**：客户端发送带唯一 `id` 的请求，服务端在处理后返回带有相同 `id` 的响应。每个请求在独立的异步工作线程中处理，WebSocket IO 线程不再阻塞；响应就绪后即时返回。

定时行为：

- 每 **1 秒** 触发 `timer` 事件——但仅推送给订阅了 `timer` 的客户端。

---

## 连接

| 属性 | 值 |
|------|-----|
| 协议 | WebSocket (ws) |
| 主机 | `127.0.0.1`（仅本地） |
| 端口 | `48521` |

```javascript
const ws = new WebSocket('ws://127.0.0.1:48521');
```

---

## 消息格式

### 请求格式

所有客户端→服务端请求均为如下 JSON 结构：

```json
{
  "type": "<请求类型>",
  "id": "<客户端生成的唯一ID>",
  "data": {
    // 参数（结构取决于 type）
  }
}
```

| 字段 | 类型 | 必需 | 说明 |
|------|------|------|------|
| `type` | string | ✅ | 请求类型，见下文各请求定义 |
| `id` | string | 推荐 | 客户端生成的唯一ID，响应中原样返回 （建议使用随机值）|
| `data` | object | 否 | 请求参数，结构取决于 type |

### Push 事件格式

Push 事件仅发送给订阅了其 `type` 的客户端（见 [订阅](#订阅)）。服务端主动推送的事件不含 `id` 字段：

```json
{
  "type": "<事件类型>",
  "data": {
    // 事件数据
  }
}
```

| 字段 | 类型 | 说明 |
|------|------|------|
| `type` | string | 事件类型 |
| `data` | object | 事件数据 |

### 响应格式

#### 成功

```json
{
  "type": "response",
  "id": "<与请求相同的id>",
  "data": {
    "success": true,
    "result": { }
  }
}
```

| 字段 | 类型 | 说明 |
|------|------|------|
| `type` | string | 固定为 `"response"` |
| `id` | string | 与请求相同的 id |
| `data.success` | bool | `true` |
| `data.result` | any | 操作返回值（查询时为数组或对象，设置时为 `{}`） |

#### 错误

```json
{
  "type": "response",
  "id": "req-1",
  "data": {
    "success": false,
    "error": "错误描述"
  }
}
```

| 字段 | 类型 | 说明 |
|------|------|------|
| `type` | string | 固定为 `"response"` |
| `id` | string | 与请求相同的 id |
| `data.success` | bool | `false` |
| `data.error` | string | 错误描述 |

---

## Push 事件（服务端 → 客户端）

### radar_update

雷达目标位置更新。

- **触发**：`OnRadarTargetPositionUpdate`
- **频率**：每个雷达目标位置变更时

```json
{
  "type": "radar_update",
  "data": {
    // RadarTarget 对象，见 [数据结构参考 — RadarTarget](#radartarget)
  }
}
```

---

### flightplan_update

飞行计划数据更新。

- **触发**：`OnFlightPlanFlightPlanDataUpdate`
- **频率**：飞行计划数据变更时

```json
{
  "type": "flightplan_update",
  "data": {
    // FlightPlan 对象，见 [数据结构参考 — FlightPlan](#flightplan)
  }
}
```

---

### flightplan_disconnect

飞行计划断开（飞机离开管制区）。

- **触发**：`OnFlightPlanDisconnect`

| 参数 | 类型 | 说明 |
|------|------|------|
| `data.callsign` | string | 断开连接的飞行计划呼号 |

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

管制员上线或位置/状态变更。

- **触发**：`OnControllerPositionUpdate`

```json
{
  "type": "controller_update",
  "data": {
    // Controller 对象，见 [数据结构参考 — Controller](#controller)
  }
}
```

---

### controller_disconnect

管制员断开连接。

- **触发**：`OnControllerDisconnect`

| 参数 | 类型 | 说明 |
|------|------|------|
| `data.callsign` | string | 管制员呼号 |
| `data.position_id` | string | 席位 ID |

```json
{
  "type": "controller_disconnect",
  "data": {
    "callsign": "ZBAA_APP",
    "position_id": "AAAP"
  }
}
```

若管制员已无效，`data` 为空对象 `{}`。

---

### controller_assigned_data

管制员修改了飞行计划的分配数据（如应答机、高度、速度等）。

- **触发**：`OnFlightPlanControllerAssignedDataUpdate`

```json
{
  "type": "controller_assigned_data",
  "data": {
    /* FlightPlan 完整对象 */
    "data_type": 1
  }
}
```

| 参数 | 类型 | 说明 |
|------|------|------|
| `data` | object | FlightPlan 完整对象 |
| `data.data_type` | int | 变更的数据类型标识（由 EuroScope 定义） |

---

### flight_strip_pushed

飞行进程单被推送给其他管制员。

- **触发**：`OnFlightPlanFlightStripPushed`

| 参数 | 类型 | 说明 |
|------|------|------|
| `data.callsign` | string | 飞行计划呼号 |
| `data.sender` | string | 发送方管制员呼号 |
| `data.target` | string | 接收方管制员呼号 |

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

私聊消息。

- **触发**：`OnCompilePrivateChat`

| 参数 | 类型 | 说明 |
|------|------|------|
| `data.sender` | string | 发送者呼号 |
| `data.receiver` | string | 接收者呼号 |
| `data.message` | string | 消息内容 |

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

频率上发送的文字聊天消息。

- **触发**：`OnCompileFrequencyChat`

| 参数 | 类型 | 说明 |
|------|------|------|
| `data.sender` | string | 发送者呼号 |
| `data.frequency` | number | 发送频率 (MHz) |
| `data.message` | string | 消息内容 |

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

收到新的 METAR 气象报文。

- **触发**：`OnNewMetarReceived`

| 参数 | 类型 | 说明 |
|------|------|------|
| `data.station` | string | 气象站四字码（如 ZBAA） |
| `data.metar` | string | 完整 METAR 报文内容 |

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

飞机机型/涂装信息更新。

- **触发**：`OnPlaneInformationUpdate`

| 参数 | 类型 | 说明 |
|------|------|------|
| `data.callsign` | string | 飞机呼号 |
| `data.livery` | string | 涂装/航空公司代码 |
| `data.plane_type` | string | 机型代码 |

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

定时器事件，每秒触发一次。

- **触发**：`OnTimer`
- **频率**：1 Hz

| 参数 | 类型 | 说明 |
|------|------|------|
| `data.counter` | int | 自插件启动以来的秒数计数 |

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

跑道活动状态变更。

- **触发**：`OnAirportRunwayActivityChanged`

```json
{
  "type": "airport_runway_activity_changed",
  "data": {}
}
```

---

## 订阅

Push 事件**默认不推送**。客户端必须订阅需要的事件类型，只有订阅的客户端才会收到对应事件；没有任何客户端订阅某事件时，该事件对应的 EuroScope 回调会被完全跳过（不做序列化与推送）。

- 订阅是**按客户端**的（每个 WebSocket 连接拥有各自的订阅集合）。
- 客户端断开连接时订阅自动清除。
- 未知的事件类型字符串会被接受但永远不会产生事件；全部 Push 事件类型见 [Push 事件（服务端 → 客户端）](#push-事件服务端--客户端)。

### subscribe

将给定的事件类型加入该客户端的订阅集合。可重复调用——再次调用并传入更多类型时追加订阅。

**请求示例：**

```json
{ "type": "subscribe", "id": "sub-1", "data": { "events": ["radar_update", "flightplan_update", "timer"] } }
```

| 参数 | 类型 | 必需 | 说明 |
|------|------|------|------|
| `data.events` | string[] | ✅ | 要订阅的事件类型 |

**响应示例：**

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

| 字段 | 类型 | 说明 |
|------|------|------|
| `data.success` | bool | `true` |
| `data.events` | string[] | 操作完成后该客户端当前的完整订阅集合 |

### unsubscribe

将给定的事件类型从该客户端的订阅集合中移除。若省略 `data.events` 或传入空数组，则**清空全部订阅**。

**请求示例：**

```json
{ "type": "unsubscribe", "id": "sub-2", "data": { "events": ["timer"] } }

// 清空全部订阅
{ "type": "unsubscribe", "id": "sub-3" }
```

| 参数 | 类型 | 必需 | 说明 |
|------|------|------|------|
| `data.events` | string[] | 否 | 要取消订阅的事件类型；省略或传 `[]` 表示清空全部 |

响应格式与 `subscribe` 相同（`success` + 当前 `events`）。

**错误示例**（`data.events` 不是数组）：

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

## 心跳（Ping / Pong）

支持但**不强制**应用层心跳：客户端可随时发送 `ping`，服务端立即回复 `pong`。可配合订阅的 `timer` 事件做连接健康检测。

**请求示例：**

```json
{ "type": "ping", "id": "hb-1" }
```

**响应示例：**

```json
{ "type": "pong", "id": "hb-1" }
```

- `id` 为可选字段；`pong` 会原样回显 `ping` 中携带的 `id`（未携带则不返回）
- 心跳在 IO 线程内联处理、立即响应，不占用并发请求配额

---

## 请求（客户端 → 服务端）

### 查询类

#### get_flightplans

获取飞行计划。不传 `callsign` 返回全部，传入则只返回匹配的那一条。

| 参数 | 类型 | 必需 | 说明 |
|------|------|------|------|
| `data.callsign` | string | 否 | 飞行计划呼号；省略则返回全部 |

**请求示例：**

```json
// 查询全部
{ "type": "get_flightplans", "id": "1" }

// 查询单个
{ "type": "get_flightplans", "id": "1", "data": { "callsign": "CES1234" } }
```

**响应示例：**

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

获取雷达目标。不传 `callsign` 返回全部。

| 参数 | 类型 | 必需 | 说明 |
|------|------|------|------|
| `data.callsign` | string | 否 | 雷达目标呼号；省略则返回全部 |

**请求示例：**

```json
{ "type": "get_radar_targets", "id": "2" }
{ "type": "get_radar_targets", "id": "2", "data": { "callsign": "CES1234" } }
```

**响应示例：**

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

获取所有在线管制员（自动包含自己）。

- **无参数**

**请求示例：**

```json
{ "type": "get_controllers", "id": "3" }
```

**响应示例：**

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

获取扇区文件元素。

| 参数 | 类型 | 必需 | 默认值 | 说明 |
|------|------|------|--------|------|
| `data.element_type` | int | 否 | `-1` | 元素类型过滤，`-1` 表示全部 |

**请求示例：**

```json
{ "type": "get_sector_elements", "id": "4", "data": { "element_type": -1 } }
```

**响应示例：**

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

获取所有语音频道。

- **无参数**

**请求示例：**

```json
{ "type": "get_voice_channels", "id": "5" }
```

**响应示例：**

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

获取当前扇区的过渡高度。

- **无参数**

**请求示例：**

```json
{ "type": "get_transition_altitude", "id": "6" }
```

**响应示例：**

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

| 响应字段 | 类型 | 说明 |
|----------|------|------|
| `result.transition_altitude` | int | 过渡高度（英尺） |

---

#### get_connection_type

获取当前连接类型。

- **无参数**

**请求示例：**

```json
{ "type": "get_connection_type", "id": "7" }
```

**响应示例：**

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

| 响应字段 | 类型 | 说明 |
|----------|------|------|
| `result.connection_type` | int | 连接类型代码 |

---

#### get_full_snapshot

获取完整当前状态快照，包含全部飞行计划、雷达目标和在线管制员。

- **无参数**

**请求示例：**

```json
{ "type": "get_full_snapshot", "id": "8" }
```

**响应示例：**

```json
{
  "type": "response",
  "id": "8",
  "data": {
    "success": true,
    "result": {
      "flightplans": [
        // FlightPlan[], 见数据结构参考
      ],
      "radar_targets": [
        // RadarTarget[], 见数据结构参考
      ],
      "controllers": [
        // Controller[], 见数据结构参考
      ]
    }
  }
}
```

| 响应字段 | 类型 | 说明 |
|----------|------|------|
| `result.flightplans` | array | 全部 FlightPlan 对象 |
| `result.radar_targets` | array | 全部 RadarTarget 对象 |
| `result.controllers` | array | 全部 Controller 对象（含自己） |

---

### 设置类 — ControllerAssignedData

以下请求操作飞行计划的 **ControllerAssignedData** 数据（管制员分配的临时数据，覆盖飞行计划原始值）。

**公共参数：** 所有此类请求均需 `data.callsign` 和目标值（通过 `data.value` 传递，字符串或整数均可）。

#### set_squawk

设置应答机编码。

| 参数 | 类型 | 必需 | 说明 |
|------|------|------|------|
| `data.callsign` | string | ✅ | 飞行计划呼号 |
| `data.value` | string | ✅ | 应答机编码（4位数字字符串） |

```json
{ "type": "set_squawk", "id": "r1", "data": { "callsign": "CES1234", "value": "5678" } }
```

---

#### set_final_altitude

设置管制员分配的最终高度（覆盖飞行计划原始最终高度）。

| 参数 | 类型 | 必需 | 说明 |
|------|------|------|------|
| `data.callsign` | string | ✅ | 飞行计划呼号 |
| `data.value` | int | ✅ | 最终高度（英尺） |

```json
{ "type": "set_final_altitude", "id": "r2", "data": { "callsign": "CES1234", "value": 35000 } }
```

---

#### set_cleared_altitude

设置许可高度。

| 参数 | 类型 | 必需 | 说明 |
|------|------|------|------|
| `data.callsign` | string | ✅ | 飞行计划呼号 |
| `data.value` | int | ✅ | 许可高度（英尺） |

```json
{ "type": "set_cleared_altitude", "id": "r3", "data": { "callsign": "CES1234", "value": 5000 } }
```

---

#### set_scratchpad

设置便签文本。

| 参数 | 类型 | 必需 | 说明 |
|------|------|------|------|
| `data.callsign` | string | ✅ | 飞行计划呼号 |
| `data.value` | string | ✅ | 便签文本 |

```json
{ "type": "set_scratchpad", "id": "r4", "data": { "callsign": "CES1234", "value": "ANYTHING" } }
```

---

#### set_speed

设置分配速度（IAS，节）。

| 参数 | 类型 | 必需 | 说明 |
|------|------|------|------|
| `data.callsign` | string | ✅ | 飞行计划呼号 |
| `data.value` | int | ✅ | 速度（节） |

```json
{ "type": "set_speed", "id": "r5", "data": { "callsign": "CES1234", "value": 250 } }
```

---

#### set_mach

设置分配马赫数。

| 参数 | 类型 | 必需 | 说明 |
|------|------|------|------|
| `data.callsign` | string | ✅ | 飞行计划呼号 |
| `data.value` | int | ✅ | 马赫数 × 100（如 78 表示 M.78） |

```json
{ "type": "set_mach", "id": "r6", "data": { "callsign": "CES1234", "value": 78 } }
```

---

#### set_rate

设置升降率（英尺/分钟）。

| 参数 | 类型 | 必需 | 说明 |
|------|------|------|------|
| `data.callsign` | string | ✅ | 飞行计划呼号 |
| `data.value` | int | ✅ | 升降率（ft/min） |

```json
{ "type": "set_rate", "id": "r7", "data": { "callsign": "CES1234", "value": 1500 } }
```

---

#### set_heading

设置分配航向（度）。

| 参数 | 类型 | 必需 | 说明 |
|------|------|------|------|
| `data.callsign` | string | ✅ | 飞行计划呼号 |
| `data.value` | int | ✅ | 航向（0-360） |

```json
{ "type": "set_heading", "id": "r8", "data": { "callsign": "CES1234", "value": 180 } }
```

---

#### set_direct_to

设置直飞导航点。

| 参数 | 类型 | 必需 | 说明 |
|------|------|------|------|
| `data.callsign` | string | ✅ | 飞行计划呼号 |
| `data.value` | string | ✅ | 导航点名称 |

```json
{ "type": "set_direct_to", "id": "r9", "data": { "callsign": "CES1234", "value": "LADIX" } }
```

---

#### set_communication_type

设置通信类型。

| 参数 | 类型 | 必需 | 说明 |
|------|------|------|------|
| `data.callsign` | string | ✅ | 飞行计划呼号 |
| `data.value` | string | ✅ | 单字符通信类型码（`V` = 语音, `T` = 文字，`R`=语音仅接收） |

```json
{ "type": "set_communication_type", "id": "r10", "data": { "callsign": "CES1234", "value": "V" } }
```

---

### 设置类 — FlightPlanData

以下请求修改飞行计划的 **FlightPlanData**（飞行计划原始数据字段）。

**公共参数：** 所有此类请求均需 `data.callsign` 和目标值（通过 `data.value` 传递）。

#### set_plan_type

设置飞行计划类型。

| 参数 | 类型 | 必需 | 说明 |
|------|------|------|------|
| `data.callsign` | string | ✅ | 飞行计划呼号 |
| `data.value` | string | ✅ | 计划类型（如 `"I"`, `"V"`） |

```json
{ "type": "set_plan_type", "id": "r11", "data": { "callsign": "CES1234", "value": "I" } }
```

---

#### set_aircraft_info

设置机型信息（ICAO 机型字符串）。

| 参数 | 类型 | 必需 | 说明 |
|------|------|------|------|
| `data.callsign` | string | ✅ | 飞行计划呼号 |
| `data.value` | string | ✅ | ICAO 机型信息字符串 |

```json
{ "type": "set_aircraft_info", "id": "r12", "data": { "callsign": "CES1234", "value": "A320/M-SDE2E3FGHIRWXY/LB1" } }
```

---

#### set_origin

设置起飞机场。

| 参数 | 类型 | 必需 | 说明 |
|------|------|------|------|
| `data.callsign` | string | ✅ | 飞行计划呼号 |
| `data.value` | string | ✅ | ICAO 四字码 |

```json
{ "type": "set_origin", "id": "r13", "data": { "callsign": "CES1234", "value": "ZBAA" } }
```

---

#### set_destination

设置目的机场。

| 参数 | 类型 | 必需 | 说明 |
|------|------|------|------|
| `data.callsign` | string | ✅ | 飞行计划呼号 |
| `data.value` | string | ✅ | ICAO 四字码 |

```json
{ "type": "set_destination", "id": "r14", "data": { "callsign": "CES1234", "value": "ZSPD" } }
```

---

#### set_alternate

设置备降机场。

| 参数 | 类型 | 必需 | 说明 |
|------|------|------|------|
| `data.callsign` | string | ✅ | 飞行计划呼号 |
| `data.value` | string | ✅ | ICAO 四字码 |

```json
{ "type": "set_alternate", "id": "r15", "data": { "callsign": "CES1234", "value": "ZSNJ" } }
```

---

#### set_remarks

设置备注（RMK/）。

| 参数 | 类型 | 必需 | 说明 |
|------|------|------|------|
| `data.callsign` | string | ✅ | 飞行计划呼号 |
| `data.value` | string | ✅ | 备注文本 |

```json
{ "type": "set_remarks", "id": "r16", "data": { "callsign": "CES1234", "value": "RMK/TCAS EQUIPPED" } }
```

---

#### set_route

设置航路字符串。

| 参数 | 类型 | 必需 | 说明 |
|------|------|------|------|
| `data.callsign` | string | ✅ | 飞行计划呼号 |
| `data.value` | string | ✅ | 完整航路字符串 |

```json
{ "type": "set_route", "id": "r17", "data": { "callsign": "CES1234", "value": "LADIX W40 YQG W34 ANRAT" } }
```

---

#### set_true_airspeed

设置真空速（节）。

| 参数 | 类型 | 必需 | 说明 |
|------|------|------|------|
| `data.callsign` | string | ✅ | 飞行计划呼号 |
| `data.value` | int | ✅ | 真空速（节） |

```json
{ "type": "set_true_airspeed", "id": "r18", "data": { "callsign": "CES1234", "value": 460 } }
```

---

#### set_departure_time

设置预计起飞时间（EOBT）。

| 参数 | 类型 | 必需 | 说明 |
|------|------|------|------|
| `data.callsign` | string | ✅ | 飞行计划呼号 |
| `data.value` | string | ✅ | 时间字符串（格式取决于 EuroScope） |

```json
{ "type": "set_departure_time", "id": "r19", "data": { "callsign": "CES1234", "value": "1200" } }
```

---

#### set_actual_departure_time

设置实际起飞时间（ATD）。

| 参数 | 类型 | 必需 | 说明 |
|------|------|------|------|
| `data.callsign` | string | ✅ | 飞行计划呼号 |
| `data.value` | string | ✅ | 时间字符串 |

```json
{ "type": "set_actual_departure_time", "id": "r20", "data": { "callsign": "CES1234", "value": "1215" } }
```

---

#### set_enroute_hours

设置航路飞行小时。

| 参数 | 类型 | 必需 | 说明 |
|------|------|------|------|
| `data.callsign` | string | ✅ | 飞行计划呼号 |
| `data.value` | string | ✅ | 小时数字符串 |

```json
{ "type": "set_enroute_hours", "id": "r21", "data": { "callsign": "CES1234", "value": "1" } }
```

---

#### set_enroute_minutes

设置航路飞行分钟。

| 参数 | 类型 | 必需 | 说明 |
|------|------|------|------|
| `data.callsign` | string | ✅ | 飞行计划呼号 |
| `data.value` | string | ✅ | 分钟数字符串 |

```json
{ "type": "set_enroute_minutes", "id": "r22", "data": { "callsign": "CES1234", "value": "30" } }
```

---

#### set_fuel_hours

设置燃油小时。

| 参数 | 类型 | 必需 | 说明 |
|------|------|------|------|
| `data.callsign` | string | ✅ | 飞行计划呼号 |
| `data.value` | string | ✅ | 小时数字符串 |

```json
{ "type": "set_fuel_hours", "id": "r23", "data": { "callsign": "CES1234", "value": "2" } }
```

---

#### set_fuel_minutes

设置燃油分钟。

| 参数 | 类型 | 必需 | 说明 |
|------|------|------|------|
| `data.callsign` | string | ✅ | 飞行计划呼号 |
| `data.value` | string | ✅ | 分钟数字符串 |

```json
{ "type": "set_fuel_minutes", "id": "r24", "data": { "callsign": "CES1234", "value": "0" } }
```

---

### 操作类

#### amend_flight_plan

提交飞行计划修改（使 FlightPlanData 的修改生效）。

| 参数 | 类型 | 必需 | 说明 |
|------|------|------|------|
| `data.callsign` | string | ✅ | 飞行计划呼号 |

```json
{ "type": "amend_flight_plan", "id": "a1", "data": { "callsign": "CES1234" } }
```

---

#### start_tracking

开始追踪飞行计划。

| 参数 | 类型 | 必需 | 说明 |
|------|------|------|------|
| `data.callsign` | string | ✅ | 飞行计划呼号 |

```json
{ "type": "start_tracking", "id": "a2", "data": { "callsign": "CES1234" } }
```

---

#### end_tracking

结束追踪飞行计划。

| 参数 | 类型 | 必需 | 说明 |
|------|------|------|------|
| `data.callsign` | string | ✅ | 飞行计划呼号 |

```json
{ "type": "end_tracking", "id": "a3", "data": { "callsign": "CES1234" } }
```

---

#### accept_handoff

接受移交。

| 参数 | 类型 | 必需 | 说明 |
|------|------|------|------|
| `data.callsign` | string | ✅ | 飞行计划呼号 |

```json
{ "type": "accept_handoff", "id": "a4", "data": { "callsign": "CES1234" } }
```

---

#### refuse_handoff

拒绝移交。

| 参数 | 类型 | 必需 | 说明 |
|------|------|------|------|
| `data.callsign` | string | ✅ | 飞行计划呼号 |

```json
{ "type": "refuse_handoff", "id": "a5", "data": { "callsign": "CES1234" } }
```

---

#### initiate_handoff

发起移交给指定管制员。

| 参数 | 类型 | 必需 | 说明 |
|------|------|------|------|
| `data.callsign` | string | ✅ | 飞行计划呼号 |
| `data.target` | string | ✅ | 目标管制员呼号 |

```json
{ "type": "initiate_handoff", "id": "a6", "data": { "callsign": "CES1234", "target": "ZBAA_TWR" } }
```

---

#### push_flight_strip

推送飞行进程单给指定管制员。

| 参数 | 类型 | 必需 | 说明 |
|------|------|------|------|
| `data.callsign` | string | ✅ | 飞行计划呼号 |
| `data.target` | string | ✅ | 目标管制员呼号 |

```json
{ "type": "push_flight_strip", "id": "a7", "data": { "callsign": "CES1234", "target": "ZBAA_TWR" } }
```

---

#### set_estimation

设置预计过点时间。

| 参数 | 类型 | 必需 | 说明 |
|------|------|------|------|
| `data.callsign` | string | ✅ | 飞行计划呼号 |
| `data.point` | string | ✅ | 导航点名称 |
| `data.time` | string | ✅ | 预计时间字符串 |

```json
{ "type": "set_estimation", "id": "a8", "data": { "callsign": "CES1234", "point": "LADIX", "time": "1230" } }
```

---

#### clear_estimation

清除预计时间。不传 `point` 则清除全部预计时间；传入 `point` 则仅清除指定导航点的预计时间。

| 参数 | 类型 | 必需 | 说明 |
|------|------|------|------|
| `data.callsign` | string | ✅ | 飞行计划呼号 |
| `data.point` | string | 否 | 导航点名称；省略则清除全部 |

```json
// 清除全部
{ "type": "clear_estimation", "id": "a9", "data": { "callsign": "CES1234" } }

// 清除指定点
{ "type": "clear_estimation", "id": "a9", "data": { "callsign": "CES1234", "point": "LADIX" } }
```

---

#### set_flight_strip_annotation

设置飞行进程单注释（第 0-8 行）。

| 参数 | 类型 | 必需 | 说明 |
|------|------|------|------|
| `data.callsign` | string | ✅ | 飞行计划呼号 |
| `data.index` | int | ✅ | 注释行索引（0-8） |
| `data.annotation` | string | ✅ | 注释文本 |

```json
{ "type": "set_flight_strip_annotation", "id": "a10", "data": { "callsign": "CES1234", "index": 0, "annotation": "PEL" } }
```

---

#### correlate

将飞行计划与雷达目标关联。

| 参数 | 类型 | 必需 | 说明 |
|------|------|------|------|
| `data.fp_callsign` | string | ✅ | 飞行计划呼号 |
| `data.rt_callsign` | string | ✅ | 雷达目标呼号 |

```json
{ "type": "correlate", "id": "a11", "data": { "fp_callsign": "CES1234", "rt_callsign": "CES1234" } }
```

---

#### uncorrelate

解除飞行计划与雷达目标的关联。

| 参数 | 类型 | 必需 | 默认值 | 说明 |
|------|------|------|--------|------|
| `data.callsign` | string | ✅ | — | 飞行计划或雷达目标呼号 |
| `data.target_type` | string | 否 | `"fp"` | 目标类型：`"fp"` 或 `"rt"` / `"radar_target"` |

```json
// 从飞行计划侧解除关联
{ "type": "uncorrelate", "id": "a12", "data": { "callsign": "CES1234", "target_type": "fp" } }

// 从雷达目标侧解除关联
{ "type": "uncorrelate", "id": "a12", "data": { "callsign": "CES1234", "target_type": "rt" } }
```

---

#### set_asel

设置当前选中飞机（ASEL）。

| 参数 | 类型 | 必需 | 默认值 | 说明 |
|------|------|------|--------|------|
| `data.callsign` | string | ✅ | — | 飞行计划或雷达目标呼号 |
| `data.asel_type` | string | 否 | `"fp"` | 目标类型：`"fp"` 或 `"rt"` / `"radar_target"` |

```json
{ "type": "set_asel", "id": "a13", "data": { "callsign": "CES1234", "asel_type": "fp" } }
```

---

#### display_message

在 EuroScope 界面显示消息。

| 参数 | 类型 | 必需 | 默认值 | 说明 |
|------|------|------|--------|------|
| `data.handler` | string | 否 | `"DataBridge"` | 消息处理器名称 |
| `data.sender` | string | 否 | — | 发送者名称 |
| `data.message` | string | 否 | — | 消息内容 |
| `data.show` | bool | 否 | `true` | 是否显示 handler 名称 |
| `data.unread` | bool | 否 | `true` | 是否标记为未读 |
| `data.busy` | bool | 否 | `true` | 繁忙时是否仍然显示 |
| `data.flash` | bool | 否 | `true` | 是否闪烁 |
| `data.confirm` | bool | 否 | `false` | 是否需要用户确认 |

```json
{
  "type": "display_message",
  "id": "a14",
  "data": {
    "sender": "外部系统",
    "message": "数据同步完成",
    "flash": true,
    "confirm": false
  }
}
```

---

#### send_command

发送原始指令到 EuroScope。

| 参数 | 类型 | 必需 | 说明 |
|------|------|------|------|
| `data.command` | string | ✅ | EuroScope 命令字符串 |

```json
{ "type": "send_command", "id": "a15", "data": { "command": ".msg PK123 Hello" } }
```

---

### 语音频道切换

所有切换操作共享相同的参数结构。`channel` 参数指定目标频道名称，若省略则使用第一个可用频道。

#### toggle_primary

切换主频道。

| 参数 | 类型 | 必需 | 说明 |
|------|------|------|------|
| `data.channel` | string | 否 | 频道名称；省略则使用第一个可用频道 |

```json
{ "type": "toggle_primary", "id": "v1", "data": { "channel": "ZBAA_APP" } }
```

---

#### toggle_atis

切换 ATIS 频道。

| 参数 | 类型 | 必需 | 说明 |
|------|------|------|------|
| `data.channel` | string | 否 | 频道名称 |

```json
{ "type": "toggle_atis", "id": "v2", "data": { "channel": "ZBAA_ATIS" } }
```

---

#### toggle_text_receive

切换文字接收。

| 参数 | 类型 | 必需 | 说明 |
|------|------|------|------|
| `data.channel` | string | 否 | 频道名称 |

```json
{ "type": "toggle_text_receive", "id": "v3", "data": { "channel": "ZBAA_APP" } }
```

---

#### toggle_text_transmit

切换文字发送。

| 参数 | 类型 | 必需 | 说明 |
|------|------|------|------|
| `data.channel` | string | 否 | 频道名称 |

```json
{ "type": "toggle_text_transmit", "id": "v4", "data": { "channel": "ZBAA_APP" } }
```

---

#### toggle_voice_receive

切换语音接收。

| 参数 | 类型 | 必需 | 说明 |
|------|------|------|------|
| `data.channel` | string | 否 | 频道名称 |

```json
{ "type": "toggle_voice_receive", "id": "v5", "data": { "channel": "ZBAA_APP" } }
```

---

#### toggle_voice_transmit

切换语音发送。

| 参数 | 类型 | 必需 | 说明 |
|------|------|------|------|
| `data.channel` | string | 否 | 频道名称 |

```json
{ "type": "toggle_voice_transmit", "id": "v6", "data": { "channel": "ZBAA_APP" } }
```

---

## 数据结构参考

### FlightPlan

飞行计划完整对象。出现在 `flightplan_update`、`controller_assigned_data`、`full_snapshot`、`get_flightplans` 中。

```jsonc
{
  "valid": true,                        // bool   — 对象是否有效
  "callsign": "CES1234",                // string — 呼号
  "pilot_name": "Leo Chen",             // string — 飞行员姓名
  "state": 2,                           // int    — 状态码
  "fp_state": 0,                        // int    — 飞行计划状态
  "simulated": false,                   // bool   — 是否为模拟机

  // --- 追踪 ---
  "tracking_controller_callsign": "ZBAA_APP",  // string — 追踪管制员呼号
  "tracking_controller_id": "AAAA",           // string — 追踪管制员 ID
  "tracking_controller_is_me": true,           // bool   — 是否为自己在追踪

  // --- 移交 ---
  "handoff_target_callsign": "ZBAA_TWR",       // string — 移交目标管制员呼号
  "handoff_target_id": "AAAT",                // string — 移交目标管制员 ID

  // --- 航路 ---
  "distance_to_destination": 450.3,     // double — 到目的地距离 (NM)
  "distance_from_origin": 23.1,        // double — 离起飞机场距离 (NM)
  "next_copx": "LADIX",                // string — 下一个 COPX 点
  "next_fir_copx": "",                 // string — 下一个 FIR 边界点
  "sector_entry_minutes": 5,           // int    — 进入扇区分钟数
  "sector_exit_minutes": 12,           // int    — 离开扇区分钟数

  // --- 标志 ---
  "ram_flag": true,                    // bool   — RAM 标志
  "clam_flag": false,                  // bool   — CLAM 标志
  "ground_state": "AIR",               // string — 地面状态 (AIR/GND/...)
  "clearence_flag": 1,                 // int    — 放行许可标志
  "text_communication": false,         // bool   — 是否文字通信

  // --- 高度 ---
  "final_altitude": 35000,              // int    — 最终高度 (ft)
  "cleared_altitude": 5000,            // int    — 许可高度 (ft)

  // --- 协调（入口）---
  "entry_coordination_point_name": "LADIX",     // string — 入口协调点
  "entry_coordination_point_state": 1,          // int    — 入口协调点状态
  "entry_coordination_altitude_state": 1,       // int    — 入口协调高度状态
  "entry_coordination_altitude": 35000,          // int    — 入口协调高度

  // --- 协调（出口）---
  "exit_coordination_point_name": "ANRAT",      // string — 出口协调点
  "exit_coordination_name_state": 1,            // int    — 出口协调点名状态
  "exit_coordination_altitude_state": 1,        // int    — 出口协调高度状态
  "exit_coordination_altitude": 35000,           // int    — 出口协调高度

  // --- 下一管制员 ---
  "coordinated_next_controller": "ZBAA_TWR",    // string — 协调的下一管制员
  "coordinated_next_controller_state": 1,       // int    — 协调状态

  // --- 子对象 ---
  "flight_plan_data":             { /* FlightPlanData */ },
  "controller_assigned_data":     { /* ControllerAssignedData */ },
  "extracted_route":              { /* ExtractedRoute */ },
  "position_predictions":         { /* PositionPredictions */ },
  "track_position":               { /* RadarTargetPositionData */ }
}
```

---

### FlightPlanData

飞行计划原始数据。

```jsonc
{
  "is_received": true,                // bool   — 是否已接收
  "is_amended": false,                // bool   — 是否已修改
  "plan_type": "I",                   // string — 计划类型 (I/V)
  "aircraft_info": "A320/M-SDE2E3FGHIRWXY/LB1",  // string — ICAO 机型字符串
  "aircraft_wtc": "M",                // char   — 尾流等级 (L/M/H/J)
  "aircraft_type": "J",               // char   — 飞机类型
  "engine_number": 2,                 // int    — 发动机数量
  "engine_type": "J",                 // char   — 发动机类型
  "capabilities": "S",                // char   — 设备能力代码
  "is_rvsm": true,                    // bool   — 是否 RVSM 能力
  "manufacturer_type": "A320",        // string — 制造商型号
  "aircraft_fp_type": "A320",         // string — 飞行计划中的机型
  "true_airspeed": 460,               // int    — 真空速 (kt)
  "origin": "ZBAA",                   // string — 起飞机场 ICAO
  "destination": "ZSPD",              // string — 目的机场 ICAO
  "alternate": "ZSNJ",                // string — 备降机场 ICAO
  "final_altitude": 35000,             // int    — 最终高度 (ft)
  "remarks": "",                      // string — 备注 (RMK/)
  "communication_type": "V",          // char   — 通信类型
  "route": "LADIX W40 YQG W34 ANRAT", // string — 航路
  "sid_name": "LADIX11D",             // string — SID 名称
  "star_name": "",                    // string — STAR 名称
  "departure_rwy": "36R",             // string — 起飞跑道
  "arrival_rwy": "",                  // string — 到达跑道
  "estimated_departure_time": "1200", // string — EOBT
  "actual_departure_time": "",        // string — ATD
  "enroute_hours": "1",               // string — 航路小时
  "enroute_minutes": "30",            // string — 航路分钟
  "fuel_hours": "2",                  // string — 燃油小时
  "fuel_minutes": "0"                 // string — 燃油分钟
}
```

---

### ControllerAssignedData

管制员分配的临时数据。

```jsonc
{
  "squawk": "1234",                   // string — 应答机编码
  "final_altitude": 9800,             // int    — 管制员分配的最终高度
  "cleared_altitude": 5000,           // int    — 许可高度
  "communication_type": "V",          // char   — 通信类型
  "scratchpad": "",                   // string — 便签
  "assigned_speed": 250,              // int    — 分配速度 (kt)
  "assigned_mach": 0,                 // int    — 分配马赫数 ×100
  "assigned_rate": 0,                 // int    — 分配升降率 (ft/min)
  "assigned_heading": 180,            // int    — 分配航向 (°)
  "direct_to": "",                    // string — 直飞点
  "flight_strip_annotations": [       // string[] — 进程单注释 (索引 0-8)
    "", "", "", "", "", "", "", "", ""
  ]
}
```

---

### ExtractedRoute

提取的航路数据。

```jsonc
{
  "points_number": 4,                 // int  — 航路点数量
  "calculated_index": 0,              // int  — 计算索引
  "assigned_index": 0,                // int  — 分配索引
  "points": [
    {
      "index": 0,                     // int    — 序号
      "name": "LADIX",                // string — 导航点名称
      "latitude": 40.123,             // double — 纬度
      "longitude": 116.456,           // double — 经度
      "airway_name": "W40",           // string — 航路名称
      "airway_classification": 1,     // int    — 航路分类
      "distance_in_minutes": 0,       // int    — 距离（分钟）
      "calculated_profile_altitude": 35000  // int — 计算剖面高度
    }
  ]
}
```

---

### PositionPredictions

位置预测数据。

```jsonc
{
  "points_number": 3,                 // int  — 预测点数量
  "points": [
    {
      "index": 0,                     // int    — 序号
      "latitude": 39.5,               // double — 纬度
      "longitude": 117.1,             // double — 经度
      "altitude": 35000,               // int    — 高度 (ft)
      "controller_id": "ZBAA_APP"     // string — 管制员 ID
    }
  ]
}
```

---

### RadarTargetPositionData

雷达目标位置数据（单个位置快照）。

```jsonc
{
  "valid": true,                      // bool   — 是否有效
  "latitude": 39.923,                 // double — 纬度
  "longitude": 116.587,               // double — 经度
  "flight_level": 350,                // int    — 高度 (FL)
  "squawk": "1234",                   // string — 应答机
  "reported_gs": 460,                 // int    — 地速 (kt)
  "reported_heading": 180,            // int    — 磁航向 (°)
  "reported_heading_true_north": 176, // int    — 真航向 (°)
  "reported_pitch": 0,                // int    — 俯仰角
  "reported_bank": 0,                 // int    — 坡度
  "radar_flags": 0,                   // int    — 雷达标志位
  "received_time": 1234567890,        // int    — 接收时间戳
  "transponder_c": true,              // bool   — Mode C 应答
  "transponder_i": false              // bool   — Mode I 应答
}
```

---

### RadarTarget

雷达目标完整对象。出现在 `radar_update`、`full_snapshot`、`get_radar_targets` 中。

```jsonc
{
  "valid": true,                      // bool   — 是否有效
  "callsign": "CES1234",              // string — 呼号
  "system_id": "RADAR1",              // string — 雷达站 ID
  "vertical_speed": 1500,             // int    — 垂直速度 (ft/min)
  "track_heading": 178,               // int    — 航迹 (°)
  "ground_speed": 460,                // int    — 地速 (kt)
  "correlated_callsign": "CES1234",   // string — 关联的飞行计划呼号（null 表示无关联）
  "position": {
    // RadarTargetPositionData — 当前位置
  },
  "position_history": [
    // RadarTargetPositionData[] — 历史位置（按时间倒序）
  ]
}
```

---

### Controller

管制员对象。出现在 `controller_update`、`full_snapshot`、`get_controllers` 中。

```jsonc
{
  "valid": true,                      // bool   — 是否有效
  "callsign": "ZBAA_APP",                // string — 管制员呼号
  "position_id": "AAAA",          // string — 席位 ID
  "identified": true,                 // bool   — 是否身份已验证
  "primary_frequency": 121.100,       // double — 主频频率 (MHz)
  "full_name": "Leo Chen",            // string — 全名
  "rating": 5,                        // int    — 管制员等级
  "facility": 1,                      // int    — 设施类型
  "sector_file": "ZBAA.sct",          // string — 扇区文件名
  "is_controller": true,              // bool   — 是否为管制员
  "position": {
    "latitude": 40.08,                // double — 纬度
    "longitude": 116.58               // double — 经度
  },
  "range": 200,                       // int    — 可视范围 (NM)
  "is_breaking": false,               // bool   — 是否正在断开
  "ongoing_able": true                // bool   — 是否可接续
}
```

---

### SectorElement

扇区元素对象。出现在 `get_sector_elements` 中。

```jsonc
{
  "valid": true,                      // bool   — 是否有效
  "name": "ZBAA_APP",                 // string — 元素名称
  "element_type": 1,                  // int    — 元素类型
  "positions": [
    { "latitude": 39.5, "longitude": 116.0 },   // 多边形顶点
    { "latitude": 40.5, "longitude": 117.0 }
  ],
  "components": ["ZBAA_APP"],         // string[] — 组件名称
  "frequency": 121.1,                 // double — 频率 (MHz)
  "runways": [
    { "name": "36R", "heading": 359 },  // 跑道
    { "name": "18L", "heading": 179 }
  ],
  "airport_name": "ZBAA",             // string — 机场名称
  "active_arrival": true,             // bool   — 进场激活
  "active_departure": false           // bool   — 离场激活
}
```

---

### VoiceChannel

语音频道对象。出现在 `get_voice_channels` 中。

```jsonc
{
  "valid": true,                      // bool   — 是否有效
  "name": "ZBAA_APP",                 // string — 频道名称
  "frequency": 121.100,               // double — 频率 (MHz)
  "voice_server": "voice.example.com",// string — 语音服务器地址
  "voice_channel": "ZBAA_APP",        // string — 语音频道 ID
  "is_primary": true,                 // bool   — 是否主频道
  "is_atis": false,                   // bool   — 是否 ATIS
  "is_text_receive_on": true,         // bool   — 文字接收是否开启
  "is_text_transmit_on": false,       // bool   — 文字发送是否开启
  "is_voice_receive_on": true,        // bool   — 语音接收是否开启
  "is_voice_transmit_on": true,       // bool   — 语音发送是否开启
  "is_voice_connected": true          // bool   — 语音是否已连接
}
```

---

## 错误处理

所有错误通过 `response` 消息返回，`data.success` 为 `false`。

### 常见错误消息

| 错误消息 | 触发条件 |
|----------|----------|
| `Missing 'type' field` | 请求缺少 `type` 字段 |
| `Invalid request: expected a JSON object` | 请求负载不是 JSON 对象 |
| `Missing 'callsign' for setter operation` | 设置操作缺少 `data.callsign` |
| `Flight plan not found: <callsign>` | 指定的飞行计划不存在 |
| `Radar target not found: <callsign>` | 指定的雷达目标不存在 |
| `Voice channel not found: <name>` | 指定的语音频道未找到 |
| `Missing 'target' controller` | `initiate_handoff` 或 `push_flight_strip` 缺少 `target` |
| `Missing 'point' or 'time'` | `set_estimation` 缺少 `point` 或 `time` |
| `Missing 'fp_callsign' or 'rt_callsign'` | `correlate` 缺少参数 |
| `Missing or invalid 'index' (0-8 required)` | `set_flight_strip_annotation` 的 `index` 无效 |
| `Missing or invalid 'value' for set_communication_type` | `set_communication_type` 的 `value` 为空或缺失 |
| `Failed to set <field>` | 设置操作执行失败（目标无效或数值被拒绝） |
| `Server busy: too many in-flight requests` | 并发请求达到上限（64） |
| `Unknown message type: <type>` | 不支持的请求类型 |
| `Invalid JSON` | 请求不是合法的 JSON（立即返回错误，不做处理） |

---

## 配置常量

| 常量 | 值 | 说明 |
|------|-----|------|
| WebSocket 端口 | `48521` | 仅监听 `127.0.0.1` |
| 并发客户端上限 | `64` | 超过后新连接被拒绝 |
| 入站消息大小上限 | `1 MB` | 超限帧以 `message_too_big` 协议错误拒绝 |
| 并发请求上限 | `64` | 超出后立即返回 `Server busy` 错误 |
| 每客户端发送队列上限 | `32 MB` | 背压保护，慢消费者丢弃帧 |
| 定时器间隔 | `1 s` | `timer` 事件频率 |
