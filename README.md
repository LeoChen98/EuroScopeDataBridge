# EuroScope Data Bridge

**Language: English | [中文](./README_CN.md)**

An EuroScope simulation ATC plugin DLL that exposes live flight data through a local WebSocket API. It lets external programs subscribe to events such as runway activity changes, radar position updates and flight plan changes, as well as actively query and modify EuroScope data.

## Architecture

```
┌───────────────────────────┐     WebSocket (ws://127.0.0.1:48521)      ┌──────────────────┐
│  EuroScope (main thread)  │ ◄──────────────────────────────────────►  │ External clients │
│                           │    JSON Push Events + Request/Response    │  (web pages /    │
│  DataBridgePlugin         │                                           │   scripts / data │
│  ├─ ES Callbacks          │                                           │   analysis tools)│
│  ├─ Per-request workers   │                                           └──────────────────┘
│  └─ FullSnapshot          │
└───────────────────────────┘
```

- **Push mode (subscription-based)**: EuroScope callback events (radar, flight plans, controllers, chat, METAR, etc.) are automatically serialized to JSON. A client must first `subscribe` to the event types it is interested in; only subscribed clients receive the matching events. When no client has subscribed to an event type, its callback is skipped entirely (no serialization, no push).
- **Pull/Request mode**: clients send JSON requests (e.g. `get_flightplans`, `get_full_snapshot`). Each request is processed asynchronously on its own worker thread, so the WebSocket IO thread never blocks; results are returned as soon as they are ready.
- **Heartbeat (optional)**: clients may send a `ping` message and the server replies with `pong` immediately — combined with the `timer` event for liveness detection.
- **Timed events**: a `timer` event (with a tick counter) fires once per second, likewise only pushed to clients that subscribed to it — handy for client-side polling or heartbeat detection.

## Tech stack

| Component | Description |
|-----------|-------------|
| C++17 | DLL compilation standard |
| Visual Studio 2022 (v143) | Toolset |
| Win32 (x86) | Target platform (EuroScope is a 32-bit process) |
| [websocketpp](https://github.com/zaphoyd/websocketpp) | WebSocket server (header-only, bundled) |
| [nlohmann/json](https://github.com/nlohmann/json) | JSON serialization/deserialization |
| vcpkg | C++ package manager |

## Building

### Prerequisites

- Visual Studio 2022 (with the "Desktop development with C++" workload)
- [vcpkg](https://github.com/microsoft/vcpkg) with the `VCPKG_ROOT` environment variable set

### Install dependencies

```bash
vcpkg install nlohmann-json:x86-windows asio:x86-windows
```

### Build

1. Open `EuroscopeDataBridge.sln` with Visual Studio 2022
2. Select `Debug | Win32` or `Release | Win32`
3. Build → Build Solution

The resulting `EuroscopeDataBridge.dll` is located in the `Debug\` or `Release\` directory.

## Installing into EuroScope

Copy `EuroscopeDataBridge.dll` to the EuroScope plugin directory, or add the DLL path in EuroScope's plugin settings. Once loaded, the plugin automatically starts the WebSocket server at `ws://127.0.0.1:48521`.

## Quick start

Connect to the WebSocket and subscribe to the event types you want before receiving real-time pushes:

```javascript
// Browser or Node.js
const ws = new WebSocket('ws://127.0.0.1:48521');

ws.onmessage = (event) => {
  const msg = JSON.parse(event.data);
  console.log(msg.type, msg.data);
};

ws.onopen = () => {
  // Subscribe to events: pushes are only delivered for subscribed types
  ws.send(JSON.stringify({
    type: 'subscribe',
    id: 'sub-1',
    data: { events: ['radar_update', 'flightplan_update', 'timer'] }
  }));
};

// Query all flight plans
ws.send(JSON.stringify({
  type: 'get_flightplans',
  id: 'req-1'
}));

// Change the squawk
ws.send(JSON.stringify({
  type: 'set_squawk',
  id: 'req-2',
  data: { callsign: 'CES1234', value: '1234' }
}));
```

> **Note**: all Push events are delivered only after subscription. `subscribe` may be called repeatedly to add more event types; `unsubscribe` removes subscriptions (omit `data.events` or pass an empty array to clear all).

For the full API documentation, see [docs/wiki.md](docs/wiki.md) (English) or [docs/wiki_CN.md](docs/wiki_CN.md) (中文). AI-agent skill: [docs/API_REFERENCE_SKILL.md](docs/API_REFERENCE_SKILL.md).

## Test client (TestProject)

The repository includes a WPF desktop test client at `tests/EuroScopeDataBridge.TestProject/` for quickly exercising the plugin's WebSocket API without writing any code.

- **Tech stack**: .NET 8 / WPF / MVVM (CommunityToolkit.Mvvm)
- **Features**:
  - Connect/disconnect to the WebSocket (default `127.0.0.1:48521`; host and port configurable), with a connection status indicator and message counter
  - Quick queries: Get Flight Plans / Get Radar Targets / Get Controllers, results shown in tables
  - Custom commands: send any JSON request (e.g. `{"type":"get_flightplans"}`)
  - Live log panel showing all received/sent messages, with a right-click "Copy JSON" action that copies a line's full raw JSON payload to the clipboard; the list auto-scrolls to the newest line while the scrollbar stays at the bottom

**Running**: make sure EuroScope has loaded the plugin DLL and started the WebSocket server, then open `EuroscopeDataBridge.sln` in Visual Studio 2022, set `EuroScopeDataBridge.TestProject` as the startup project and run it (or run `dotnet run --project tests/EuroScopeDataBridge.TestProject`, requires the .NET 8 SDK).

## Project structure

```
EuroscopeDataBridge/
├── src/
│   ├── dllmain.cpp / dllmain.h       # DLL entry point, plugin init/exit
│   ├── Plugin.h / Plugin.cpp         # Main plugin class, ES callbacks
│   ├── WebSocketServer.h / .cpp      # WebSocket server wrapper
│   ├── Serializer.h / .cpp           # ES objects → JSON serialization
│   ├── Handlers.h / .cpp             # Client request routing/handling
│   ├── Constants.h                   # Message types, JSON keys, port, etc.
│   └── ThreadSafeQueue.h             # Thread-safe queue
├── tests/
│   └── EuroScopeDataBridge.TestProject/  # WPF test client (.NET 8)
├── third_party/
│   ├── EuroScopePlugIn/              # EuroScope SDK headers and libs
│   ├── websocketpp/                  # WebSocket server (header-only)
│   └── nlohmann/                     # nlohmann/json headers
├── docs/
│   ├── wiki.md                       # API documentation (English)
│   └── wiki_CN.md                    # API documentation (中文)
├── EuroscopeDataBridge.sln           # VS solution
├── EuroscopeDataBridge.vcxproj       # VS project file
├── vcpkg.json                        # vcpkg dependency manifest
├── LICENSE                           # MIT License
├── README.md                         # README (English)
└── README_CN.md                      # README (中文)
```

## License

MIT License © 2026 Leo Chen — see [LICENSE](LICENSE).

Third-party components and their licenses are listed in [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).
