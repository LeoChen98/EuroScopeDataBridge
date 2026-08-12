using System;
using System.Collections.Generic;
using System.IO;
using System.Net.WebSockets;
using System.Text;
using System.Text.Json;
using System.Threading;
using System.Threading.Tasks;

namespace EuroScopeDataBridge.TestProject.Services;

/// <summary>
/// WebSocket client for connecting to EuroScopeDataBridge server.
/// </summary>
public class WebSocketService : IDisposable
{
    private ClientWebSocket? _ws;
    private CancellationTokenSource? _cts;
    private readonly string _url;
    private readonly int _port;

    public event Action<string>? MessageReceived;
    public event Action<string>? LogMessage;
    public event Action<bool>? ConnectionChanged;

    public bool IsConnected => _ws?.State == WebSocketState.Open;

    public WebSocketService(string host = "127.0.0.1", int port = 48521)
    {
        _url = host;
        _port = port;
    }

    public async Task ConnectAsync()
    {
        try
        {
            _ws?.Dispose();
            _cts?.Cancel();
            _cts?.Dispose();

            _ws = new ClientWebSocket();
            _cts = new CancellationTokenSource();

            var uri = new Uri($"ws://{_url}:{_port}");
            LogMessage?.Invoke($"Connecting to {uri}...");

            await _ws.ConnectAsync(uri, _cts.Token);
            ConnectionChanged?.Invoke(true);
            LogMessage?.Invoke("Connected successfully.");

            // Start receiving messages
            _ = ReceiveLoopAsync();
        }
        catch (Exception ex)
        {
            LogMessage?.Invoke($"Connection failed: {ex.Message}");
            ConnectionChanged?.Invoke(false);
        }
    }

    public async Task DisconnectAsync()
    {
        try
        {
            if (_ws?.State == WebSocketState.Open)
            {
                await _ws.CloseAsync(WebSocketCloseStatus.NormalClosure, "Client closing", CancellationToken.None);
            }
        }
        catch { /* ignore close errors */ }
        finally
        {
            _cts?.Cancel();
            ConnectionChanged?.Invoke(false);
            LogMessage?.Invoke("Disconnected.");
        }
    }

    public async Task SendAsync(string message)
    {
        if (_ws?.State != WebSocketState.Open)
        {
            LogMessage?.Invoke("Cannot send: not connected.");
            return;
        }

        try
        {
            var bytes = Encoding.UTF8.GetBytes(message);
            await _ws.SendAsync(new ArraySegment<byte>(bytes), WebSocketMessageType.Text, true, _cts!.Token);
            LogMessage?.Invoke($"Sent: {message}");
        }
        catch (Exception ex)
        {
            LogMessage?.Invoke($"Send failed: {ex.Message}");
        }
    }

    public async Task<string?> SendRequestAsync(string requestType, object? data = null)
    {
        var request = new Models.BridgeRequest
        {
            Type = requestType,
            Id = Guid.NewGuid().ToString("N")[..8],
            Data = data switch
            {
                Dictionary<string, object> dict => dict,
                not null => new Dictionary<string, object> { ["value"] = data },
                _ => null
            }
        };
        var json = JsonSerializer.Serialize(request);
        await SendAsync(json);
        return request.Id;
    }

    private async Task ReceiveLoopAsync()
    {
        var buffer = new byte[12*1024*1024];
        // Accumulate multi-segment messages (WebSocket may split a large message
        // across multiple ReceiveAsync calls; EndOfMessage signals completion).
        using var messageStream = new MemoryStream();

        try
        {
            while (_ws?.State == WebSocketState.Open && !_cts!.IsCancellationRequested)
            {
                var result = await _ws.ReceiveAsync(new ArraySegment<byte>(buffer), _cts.Token);

                if (result.MessageType == WebSocketMessageType.Close)
                {
                    LogMessage?.Invoke("Server closed connection.");
                    await _ws.CloseAsync(WebSocketCloseStatus.NormalClosure, "Ack", CancellationToken.None);
                    break;
                }

                if (result.MessageType == WebSocketMessageType.Text)
                {
                    messageStream.Write(buffer, 0, result.Count);

                    if (result.EndOfMessage)
                    {
                        // Full message received — decode and dispatch
                        messageStream.TryGetBuffer(out var segment);
                        var text = Encoding.UTF8.GetString(segment.Array!, segment.Offset, segment.Count);
                        MessageReceived?.Invoke(text);

                        messageStream.SetLength(0); // reset for next message
                    }
                }
            }
        }
        catch (OperationCanceledException) { }
        catch (Exception ex)
        {
            LogMessage?.Invoke($"Receive error: {ex.Message}");
        }
        finally
        {
            ConnectionChanged?.Invoke(false);
        }
    }

    public void Dispose()
    {
        _cts?.Cancel();
        _ws?.Dispose();
        _cts?.Dispose();
    }
}
