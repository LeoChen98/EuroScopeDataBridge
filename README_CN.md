# EuroScope Data Bridge

[English](./README.md)

EuroScope 模拟飞行管制插件 DLL，通过本地 WebSocket API 对外暴露实时飞行数据，支持外部程序订阅跑道活动状态变化、雷达位置更新、飞行计划变更等事件，以及主动查询和修改 EuroScope 数据。

## 架构

```
┌─────────────────────┐     WebSocket (ws://127.0.0.1:48521)     ┌──────────────────┐
│  EuroScope (主线程)  │ ◄──────────────────────────────────────► │  外部客户端        │
│                      │    JSON Push Events + Request/Response    │  (网页 / 脚本 /   │
│  DataBridgePlugin    │                                          │   数据分析工具)    │
│  ├─ ES Callbacks     │                                          └──────────────────┘
│  ├─ OnTimer → Drain  │
│  └─ FullSnapshot     │
└──────────────────────┘
```

- **Push（订阅推送）**：EuroScope 回调事件（雷达、飞行计划、管制员、聊天、METAR 等）自动序列化为 JSON。客户端需先用 `subscribe` 请求订阅感兴趣的事件类型，此后仅订阅的客户端会收到对应事件；没有任何客户端订阅某事件时，该事件对应的回调会被跳过（不做序列化与推送）。
- **Pull/Request（拉/请求模式）**：客户端发送 JSON 请求（如 `get_flightplans`、`get_full_snapshot`），在 EuroScope 主线程上处理，结果通过 WebSocket 返回。
- **定时事件**：`timer` 事件（含 tick 计数器）每秒触发一次，同样只在客户端订阅后推送，方便客户端做定时轮询或心跳检测。

## 技术栈

| 组件 | 说明 |
|------|------|
| C++17 | DLL 编译标准 |
| Visual Studio 2022 (v143) | 工具集 |
| Win32 (x86) | 目标平台（EuroScope 为 32 位进程） |
| [websocketpp](https://github.com/zaphoyd/websocketpp) | WebSocket 服务端（头文件内置） |
| [nlohmann/json](https://github.com/nlohmann/json) | JSON 序列化/反序列化 |
| vcpkg | C++ 包管理 |

## 编译

### 前置要求

- Visual Studio 2022（含 "使用 C++ 的桌面开发" 工作负载）
- [vcpkg](https://github.com/microsoft/vcpkg) 并设置环境变量 `VCPKG_ROOT`

### 安装依赖

```bash
vcpkg install nlohmann-json:x86-windows asio:x86-windows
```

### 构建

1. 用 Visual Studio 2022 打开 `EuroscopeDataBridge.sln`
2. 选择 `Debug | Win32` 或 `Release | Win32`
3. 生成 → 生成解决方案

生成的 `EuroscopeDataBridge.dll` 位于 `Debug\` 或 `Release\` 目录。

## 安装到 EuroScope

将 `EuroscopeDataBridge.dll` 复制到 EuroScope 插件目录，或在 EuroScope 的插件设置中添加该 DLL 路径。插件加载后会自动在 `ws://127.0.0.1:48521` 启动 WebSocket 服务。

## 快速开始

连接 WebSocket 后，先订阅需要的事件类型，才能收到对应的实时推送：

```javascript
// 浏览器或 Node.js
const ws = new WebSocket('ws://127.0.0.1:48521');

ws.onmessage = (event) => {
  const msg = JSON.parse(event.data);
  console.log(msg.type, msg.data);
};

ws.onopen = () => {
  // 订阅事件：订阅后才会收到对应类型的推送
  ws.send(JSON.stringify({
    type: 'subscribe',
    id: 'sub-1',
    data: { events: ['radar_update', 'flightplan_update', 'timer'] }
  }));
};

// 查询所有飞行计划
ws.send(JSON.stringify({
  type: 'get_flightplans',
  id: 'req-1'
}));

// 修改应答机
ws.send(JSON.stringify({
  type: 'set_squawk',
  id: 'req-2',
  data: { callsign: 'CES1234', value: '1234' }
}));
```

> **注意**：所有 Push 事件均需订阅后才推送。`subscribe` 可重复调用以追加订阅；`unsubscribe` 用于取消订阅（`data.events` 缺省或为空数组时表示取消全部订阅）。

详细的 API 文档请参阅 [docs/wiki_CN.md](docs/wiki_CN.md)（中文）或 [docs/wiki.md](docs/wiki.md)（English）。

## 测试客户端（TestProject）

仓库附带一个 WPF 桌面测试客户端 `tests/EuroScopeDataBridge.TestProject/`，无需编写代码即可快速验证插件的 WebSocket API。

- **技术栈**：.NET 8 / WPF / MVVM（CommunityToolkit.Mvvm）
- **功能**：
  - 连接/断开 WebSocket（默认 `127.0.0.1:48521`，Host/Port 可配置），含连接状态指示灯与消息计数
  - 快捷查询：Get Flight Plans / Get Radar Targets / Get Controllers，结果以表格展示
  - 自定义命令：输入任意 JSON 请求（如 `{"type":"get_flightplans"}`）并发送
  - 实时日志面板：显示所有收发的消息

**运行方法**：确保 EuroScope 已加载插件 DLL 并启动 WebSocket 服务，然后用 Visual Studio 2022 打开 `EuroscopeDataBridge.sln`，将 `EuroScopeDataBridge.TestProject` 设为启动项目运行（或执行 `dotnet run --project tests/EuroScopeDataBridge.TestProject`，需 .NET 8 SDK）。

## 项目结构

```
EuroscopeDataBridge/
├── src/
│   ├── dllmain.cpp / dllmain.h       # DLL 入口点，插件初始化/退出
│   ├── Plugin.h / Plugin.cpp         # 主插件类，ES 回调实现
│   ├── WebSocketServer.h / .cpp      # WebSocket 服务端封装
│   ├── Serializer.h / .cpp           # ES 对象 → JSON 序列化
│   ├── Handlers.h / .cpp             # 客户端请求路由处理
│   ├── Constants.h                   # 消息类型、JSON key、端口等常量
│   └── ThreadSafeQueue.h             # 线程安全队列
├── tests/
│   └── EuroScopeDataBridge.TestProject/  # WPF 测试客户端（.NET 8）
├── third_party/
│   ├── EuroScopePlugIn/              # EuroScope SDK 头文件与库
│   ├── websocketpp/                  # WebSocket 服务端（header-only）
│   └── nlohmann/                     # nlohmann/json 头文件
├── docs/
│   ├── wiki_CN.md                    # API 文档（中文）
│   └── wiki.md                       # API 文档（English）
├── EuroscopeDataBridge.sln           # VS 解决方案
├── EuroscopeDataBridge.vcxproj       # VS 项目文件
├── vcpkg.json                        # vcpkg 依赖清单
├── LICENSE                           # MIT License
├── README_CN.md                      # README（中文）
└── README.md                         # README（English）
```

## 许可证

MIT License © 2026 Leo Chen — 详见 [LICENSE](LICENSE)。
