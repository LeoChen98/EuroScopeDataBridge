namespace EuroScopeDataBridge.TestProject.Models;

/// <summary>
/// A single line in the log list. <see cref="Text"/> is what is displayed
/// (timestamped, possibly truncated); <see cref="JsonText"/> holds the full
/// raw JSON payload the line originated from, when one exists (null otherwise).
/// </summary>
public class LogEntry
{
    public LogEntry(string text, string? jsonText = null)
    {
        Text = text;
        JsonText = jsonText;
    }

    public string Text { get; }

    /// <summary>Full raw JSON associated with this log line, or null when the line has none.</summary>
    public string? JsonText { get; }
}
