using System.Collections.Generic;
using System.Text.Json.Serialization;

namespace EuroScopeDataBridge.TestProject.Models;

/// <summary>
/// Wrapper message received from EuroScopeDataBridge WebSocket server.
/// </summary>
public class BridgeMessage
{
    [JsonPropertyName("type")]
    public string Type { get; set; } = string.Empty;

    [JsonPropertyName("id")]
    public string? Id { get; set; }

    [JsonPropertyName("data")]
    public System.Text.Json.JsonElement? Data { get; set; }

    [JsonPropertyName("success")]
    public bool? Success { get; set; }

    [JsonPropertyName("error")]
    public string? Error { get; set; }
}

/// <summary>
/// Flight plan data matching the bridge JSON schema.
/// </summary>
public class FlightPlanData
{
    [JsonPropertyName("callsign")]
    public string Callsign { get; set; } = string.Empty;

    [JsonPropertyName("origin")]
    public string Origin { get; set; } = string.Empty;

    [JsonPropertyName("destination")]
    public string Destination { get; set; } = string.Empty;

    [JsonPropertyName("alternate")]
    public string Alternate { get; set; } = string.Empty;

    [JsonPropertyName("route")]
    public string Route { get; set; } = string.Empty;

    [JsonPropertyName("aircraft_type")]
    public string AircraftType { get; set; } = string.Empty;

    [JsonPropertyName("squawk")]
    public string Squawk { get; set; } = string.Empty;

    [JsonPropertyName("cleared_altitude")]
    public int ClearedAltitude { get; set; }

    [JsonPropertyName("final_altitude")]
    public int FinalAltitude { get; set; }

    [JsonPropertyName("scratchpad")]
    public string Scratchpad { get; set; } = string.Empty;

    [JsonPropertyName("speed")]
    public int Speed { get; set; }

    [JsonPropertyName("heading")]
    public int Heading { get; set; }
}

/// <summary>
/// Radar target data matching the bridge JSON schema.
/// </summary>
public class RadarTargetData
{
    [JsonPropertyName("callsign")]
    public string Callsign { get; set; } = string.Empty;

    [JsonPropertyName("system_id")]
    public string SystemId { get; set; } = string.Empty;

    [JsonPropertyName("vertical_speed")]
    public int VerticalSpeed { get; set; }

    [JsonPropertyName("track_heading")]
    public double TrackHeading { get; set; }

    [JsonPropertyName("ground_speed")]
    public int GroundSpeed { get; set; }

    [JsonPropertyName("correlated_callsign")]
    public string? CorrelatedCallsign { get; set; }

    [JsonPropertyName("position")]
    public RadarPositionData? Position { get; set; }
}

/// <summary>
/// Nested position data within a radar target.
/// </summary>
public class RadarPositionData
{
    [JsonPropertyName("valid")]
    public bool Valid { get; set; }

    [JsonPropertyName("latitude")]
    public double Latitude { get; set; }

    [JsonPropertyName("longitude")]
    public double Longitude { get; set; }

    [JsonPropertyName("flight_level")]
    public int FlightLevel { get; set; }

    [JsonPropertyName("reported_gs")]
    public int ReportedGs { get; set; }

    [JsonPropertyName("reported_heading")]
    public int ReportedHeading { get; set; }

    [JsonPropertyName("vertical_rate")]
    public int VerticalRate { get; set; }
}

/// <summary>
/// Controller data matching the bridge JSON schema.
/// </summary>
public class ControllerData
{
    [JsonPropertyName("callsign")]
    public string Callsign { get; set; } = string.Empty;

    [JsonPropertyName("full_name")]
    public string FullName { get; set; } = string.Empty;

    [JsonPropertyName("primary_frequency")]
    public double PrimaryFrequency { get; set; }

    [JsonPropertyName("facility")]
    public int Facility { get; set; }

    [JsonPropertyName("rating")]
    public int Rating { get; set; }

    [JsonPropertyName("position_id")]
    public string PositionId { get; set; } = string.Empty;

    [JsonPropertyName("is_controller")]
    public bool IsController { get; set; }

    [JsonPropertyName("range")]
    public int Range { get; set; }

    [JsonPropertyName("sector_file")]
    public string SectorFile { get; set; } = string.Empty;

    [JsonPropertyName("is_breaking")]
    public bool IsBreaking { get; set; }

    [JsonPropertyName("ongoing_able")]
    public bool IsOngoingAble { get; set; }
}

/// <summary>
/// Request message sent to EuroScopeDataBridge.
/// </summary>
public class BridgeRequest
{
    [JsonPropertyName("type")]
    public string Type { get; set; } = string.Empty;

    [JsonPropertyName("id")]
    public string? Id { get; set; }

    [JsonPropertyName("data")]
    public Dictionary<string, object>? Data { get; set; }
}
