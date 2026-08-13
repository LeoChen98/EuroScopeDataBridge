using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Linq;
using System.Text.Json;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Input;
using CommunityToolkit.Mvvm.ComponentModel;
using CommunityToolkit.Mvvm.Input;
using EuroScopeDataBridge.TestProject.Models;
using EuroScopeDataBridge.TestProject.Services;

namespace EuroScopeDataBridge.TestProject.ViewModels;

public partial class MainViewModel : ObservableObject
{
    private readonly WebSocketService _wsService;

    // Tracks outstanding request ids so a response is routed to the table that
    // matches the request that produced it (flight plans / radar targets /
    // controllers must not be mixed, they only share the "callsign" field).
    private readonly Dictionary<string, string> _pendingRequestTypes = new();

    [ObservableProperty]
    private string _host = "127.0.0.1";

    [ObservableProperty]
    private int _port = 48521;

    [ObservableProperty]
    [NotifyPropertyChangedFor(nameof(IsNotConnected))]
    private bool _isConnected;

    /// <summary>Inverse of <see cref="IsConnected"/>; used by the Connect button's
    /// IsEnabled binding. Raised whenever IsConnected changes.</summary>
    public bool IsNotConnected => !IsConnected;


    [ObservableProperty]
    private string _customCommand = "{\"type\":\"get_flightplans\"}";

    [ObservableProperty]
    private string _statusText = "Disconnected";

    [ObservableProperty]
    private ObservableCollection<string> _logEntries = new();

    [ObservableProperty]
    private ObservableCollection<FlightPlanData> _flightPlans = new();

    [ObservableProperty]
    private ObservableCollection<RadarTargetData> _radarTargets = new();

    [ObservableProperty]
    private ObservableCollection<ControllerData> _controllers = new();

    [ObservableProperty]
    private int _messageCount;

    [ObservableProperty]
    private FlightPlanData? _selectedFlightPlan;

    [ObservableProperty]
    private RadarTargetData? _selectedRadarTarget;

    [ObservableProperty]
    private ControllerData? _selectedController;

    public MainViewModel()
    {
        _wsService = new WebSocketService();
        _wsService.MessageReceived += OnMessageReceived;
        _wsService.LogMessage += OnLogMessage;
        _wsService.ConnectionChanged += OnConnectionChanged;
    }

    [RelayCommand]
    private async Task ConnectAsync()
    {
        _wsService.Dispose();
        var svc = new WebSocketService(Host, Port);
        svc.MessageReceived += OnMessageReceived;
        svc.LogMessage += OnLogMessage;
        svc.ConnectionChanged += OnConnectionChanged;

        // Reassign through reflection since field is readonly
        typeof(MainViewModel)
            .GetField("_wsService", System.Reflection.BindingFlags.NonPublic | System.Reflection.BindingFlags.Instance)!
            .SetValue(this, svc);

        await svc.ConnectAsync();
    }

    [RelayCommand]
    private async Task DisconnectAsync()
    {
        await _wsService.DisconnectAsync();
    }

    [RelayCommand]
    private async Task SendCustomAsync()
    {
        if (!string.IsNullOrWhiteSpace(CustomCommand))
        {
            await _wsService.SendAsync(CustomCommand);
        }
    }

    [RelayCommand]
    private async Task GetFlightPlansAsync()
    {
        var id = await _wsService.SendRequestAsync("get_flightplans");
        if (!string.IsNullOrEmpty(id))
            _pendingRequestTypes[id] = "flightplans";
        AddLog("→ Requested flight plans");
    }

    [RelayCommand]
    private async Task GetRadarTargetsAsync()
    {
        var id = await _wsService.SendRequestAsync("get_radar_targets");
        if (!string.IsNullOrEmpty(id))
            _pendingRequestTypes[id] = "radar_targets";
        AddLog("→ Requested radar targets");
    }

    [RelayCommand]
    private async Task GetControllersAsync()
    {
        var id = await _wsService.SendRequestAsync("get_controllers");
        if (!string.IsNullOrEmpty(id))
            _pendingRequestTypes[id] = "controllers";
        AddLog("→ Requested controllers");
    }

    [RelayCommand]
    private void ClearLog()
    {
        Application.Current.Dispatcher.Invoke(() => LogEntries.Clear());
    }

    private void OnMessageReceived(string json)
    {
        Application.Current.Dispatcher.Invoke(() =>
        {
            MessageCount++;
            try
            {
                var msg = JsonSerializer.Deserialize<BridgeMessage>(json);
                if (msg == null) return;
                AddLog($"← [{msg.Type}]: {Truncate(json, 200)}");
                switch (msg.Type)
                {
                    case "flightplan_update":
                        if (msg.Data.HasValue && msg.Data.Value.ValueKind == JsonValueKind.Object)
                        {
                            var fp = JsonSerializer.Deserialize<FlightPlanData>(msg.Data.Value.GetRawText());
                            if (fp != null)
                                UpsertFlightPlan(fp);
                        }
                        break;

                    case "full_snapshot":
                        if (msg.Data.HasValue)
                            ProcessSnapshotData(msg.Data.Value);
                        break;

                    case "radar_update":
                        if (msg.Data.HasValue && msg.Data.Value.ValueKind == JsonValueKind.Object)
                        {
                            var rt = JsonSerializer.Deserialize<RadarTargetData>(msg.Data.Value.GetRawText());
                            if (rt != null)
                                UpsertRadarTarget(rt);
                        }
                        break;

                    case "controller_update":
                        if (msg.Data.HasValue && msg.Data.Value.ValueKind == JsonValueKind.Object)
                        {
                            var c = JsonSerializer.Deserialize<ControllerData>(msg.Data.Value.GetRawText());
                            if (c != null)
                                UpsertController(c);
                        }
                        break;

                    case "response":
                        if (msg.Data.HasValue)
                            ProcessResponseData(msg.Id, msg.Data.Value);
                        break;

                    case "error":
                        AddLog($"ERROR: {msg.Error}");
                        break;

                    default:

                        break;
                }
            }
            catch (Exception ex)
            {
                AddLog($"Parse error: {ex.Message}");
            }
        });
    }

    private void ProcessSnapshotData(JsonElement data)
    {
        // If it's a full_snapshot, data contains sub-collections
        if (data.TryGetProperty("flightplans", out var fps))
        {
            var list = JsonSerializer.Deserialize<ObservableCollection<FlightPlanData>>(fps.GetRawText());
            if (list != null)
            {
                FlightPlans = list;
                OnPropertyChanged(nameof(FlightPlans));
            }
        }
        if (data.TryGetProperty("radar_targets", out var rts))
        {
            var list = JsonSerializer.Deserialize<ObservableCollection<RadarTargetData>>(rts.GetRawText());
            if (list != null)
            {
                RadarTargets = list;
                OnPropertyChanged(nameof(RadarTargets));
            }
        }
        if (data.TryGetProperty("controllers", out var ctrs))
        {
            var list = JsonSerializer.Deserialize<ObservableCollection<ControllerData>>(ctrs.GetRawText());
            if (list != null)
            {
                Controllers = list;
                OnPropertyChanged(nameof(Controllers));
            }
        }
    }

    private void ProcessResponseData(string? id, JsonElement data)
    {
        // Response data wraps the actual result: {"success": true, "result": [...]}
        if (!data.TryGetProperty("result", out var result))
        {
            AddLog($"← Response received without 'result' field");
            return;
        }

        // Route the result to the table matching the request that produced it.
        // The three collection types share the "callsign" field, so a blind
        // try-every-type approach would mix radar targets into the flight plan
        // table (and vice versa).
        if (id != null && _pendingRequestTypes.TryGetValue(id, out var requestType))
        {
            _pendingRequestTypes.Remove(id);
            switch (requestType)
            {
                case "flightplans":
                    TryDeserializeCollection<FlightPlanData>(result, FlightPlans, nameof(FlightPlans));
                    break;
                case "radar_targets":
                    TryDeserializeCollection<RadarTargetData>(result, RadarTargets, nameof(RadarTargets));
                    break;
                case "controllers":
                    TryDeserializeCollection<ControllerData>(result, Controllers, nameof(Controllers));
                    break;
                default:
                    AddLog($"← Response for untracked request type '{requestType}'");
                    break;
            }
        }
        else
        {
            AddLog($"← Response (id='{id ?? ""}') without tracked request; ignored");
        }

        AddLog($"← Response received ({result.ValueKind})");
    }

    private void TryDeserializeCollection<T>(JsonElement data, ObservableCollection<T> target, string propName)
    {
        try
        {
            if (data.ValueKind == JsonValueKind.Array)
            {
                var list = JsonSerializer.Deserialize<ObservableCollection<T>>(data.GetRawText());
                if (list != null && list.Count > 0)
                {
                    // Clear and repopulate
                    target.Clear();
                    foreach (var item in list)
                        target.Add(item);
                    OnPropertyChanged(propName);
                    AddLog($"← Received {list.Count} {typeof(T).Name}(s)");
                }
            }
        }
        catch { /* type mismatch, ignore */ }
    }

    private void OnLogMessage(string msg)
    {
        Application.Current.Dispatcher.Invoke(() => AddLog(msg));
    }

    private void OnConnectionChanged(bool connected)
    {
        Application.Current.Dispatcher.Invoke(async () =>
        {
            IsConnected = connected;
            StatusText = connected ? "Connected" : "Disconnected";
            if (connected)
            {
                // Push events are subscription-based: subscribe to the event
                // types this test client consumes.
                await _wsService.SendRequestAsync("subscribe", new Dictionary<string, object>
                {
                    ["events"] = new[]
                    {
                        "radar_update",
                        "flightplan_update",
                        "flightplan_disconnect",
                        "controller_update",
                        "controller_disconnect"
                    }
                });
            }
        });
    }

    private void AddLog(string entry)
    {
        var timestamp = DateTime.Now.ToString("HH:mm:ss.fff");
        LogEntries.Add($"[{timestamp}] {entry}");
    }

    private void UpsertFlightPlan(FlightPlanData fp)
    {
        var existing = FlightPlans.FirstOrDefault(f => f.Callsign == fp.Callsign);
        if (existing != null)
        {
            var idx = FlightPlans.IndexOf(existing);
            FlightPlans[idx] = fp;
        }
        else
        {
            FlightPlans.Add(fp);
        }
    }

    private void UpsertRadarTarget(RadarTargetData rt)
    {
        var existing = RadarTargets.FirstOrDefault(r => r.Callsign == rt.Callsign);
        if (existing != null)
        {
            var idx = RadarTargets.IndexOf(existing);
            RadarTargets[idx] = rt;
        }
        else
        {
            RadarTargets.Add(rt);
        }
    }

    private void UpsertController(ControllerData c)
    {
        var existing = Controllers.FirstOrDefault(ct => ct.Callsign == c.Callsign);
        if (existing != null)
        {
            var idx = Controllers.IndexOf(existing);
            Controllers[idx] = c;
        }
        else
        {
            Controllers.Add(c);
        }
    }

    private static string Truncate(string value, int maxLength) =>
        value.Length <= maxLength ? value : value[..maxLength] + "...";
}
