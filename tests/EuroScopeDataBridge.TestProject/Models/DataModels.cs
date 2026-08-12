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
///
/// The bridge serializes a flight plan as a flat top-level object with two
/// nested sub-objects (see docs/wiki_CN.md → FlightPlan / FlightPlanData /
/// ControllerAssignedData):
///   - top-level:            callsign, final_altitude, cleared_altitude, ...
///   - flight_plan_data:     origin, destination, alternate, route,
///                           aircraft_type, ...
///   - controller_assigned_data: squawk, scratchpad, assigned_speed,
///                           assigned_heading, ...
/// The nested fields are surfaced as flat read-only properties so the DataGrid
/// columns map 1:1 to the real packet fields.
/// </summary>
public class FlightPlanData
{
    [JsonPropertyName("callsign")]
    public string Callsign { get; set; } = string.Empty;

    [JsonPropertyName("cleared_altitude")]
    public int ClearedAltitude { get; set; }

    [JsonPropertyName("final_altitude")]
    public int FinalAltitude { get; set; }

    [JsonPropertyName("flight_plan_data")]
    public FlightPlanRawData? FlightPlanRawData { get; set; }

    [JsonPropertyName("controller_assigned_data")]
    public ControllerAssignedData? ControllerAssignedData { get; set; }

    // --- Flat display properties mapped from the nested sub-objects ---

    /// <summary>Maps to flight_plan_data.origin.</summary>
    public string Origin => FlightPlanRawData?.Origin ?? string.Empty;

    /// <summary>Maps to flight_plan_data.destination.</summary>
    public string Destination => FlightPlanRawData?.Destination ?? string.Empty;

    /// <summary>Maps to flight_plan_data.alternate.</summary>
    public string Alternate => FlightPlanRawData?.Alternate ?? string.Empty;

    /// <summary>Maps to flight_plan_data.route.</summary>
    public string Route => FlightPlanRawData?.Route ?? string.Empty;

    /// <summary>Maps to flight_plan_data.aircraft_type.</summary>
    public string AircraftType => FlightPlanRawData?.AircraftType ?? string.Empty;

    /// <summary>Maps to controller_assigned_data.squawk.</summary>
    public string Squawk => ControllerAssignedData?.Squawk ?? string.Empty;

    /// <summary>Maps to controller_assigned_data.scratchpad.</summary>
    public string Scratchpad => ControllerAssignedData?.Scratchpad ?? string.Empty;

    /// <summary>Maps to controller_assigned_data.assigned_speed.</summary>
    public int Speed => ControllerAssignedData?.AssignedSpeed ?? 0;

    /// <summary>Maps to controller_assigned_data.assigned_heading.</summary>
    public int Heading => ControllerAssignedData?.AssignedHeading ?? 0;
}

/// <summary>
/// flight_plan_data sub-object of a flight plan (only fields consumed by the UI).
/// </summary>
public class FlightPlanRawData
{
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
}

/// <summary>
/// controller_assigned_data sub-object of a flight plan (only fields consumed by the UI).
/// </summary>
public class ControllerAssignedData
{
    [JsonPropertyName("squawk")]
    public string Squawk { get; set; } = string.Empty;

    [JsonPropertyName("scratchpad")]
    public string Scratchpad { get; set; } = string.Empty;

    [JsonPropertyName("assigned_speed")]
    public int AssignedSpeed { get; set; }

    [JsonPropertyName("assigned_heading")]
    public int AssignedHeading { get; set; }
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

    // --- Flat display properties mapped from the nested position object ---

    /// <summary>Maps to position.latitude.</summary>
    public double Latitude => Position?.Latitude ?? 0;

    /// <summary>Maps to position.longitude.</summary>
    public double Longitude => Position?.Longitude ?? 0;

    /// <summary>Maps to position.flight_level.</summary>
    public int FlightLevel => Position?.FlightLevel ?? 0;
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
