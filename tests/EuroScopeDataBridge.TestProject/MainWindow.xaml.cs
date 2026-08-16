using System;
using System.Collections.Specialized;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;
using System.Windows.Media;

namespace EuroScopeDataBridge.TestProject;

public partial class MainWindow : Window
{
    private ScrollViewer? _logScrollViewer;
    private bool _logPinnedToBottom = true;

    public MainWindow()
    {
        InitializeComponent();
        var viewModel = new ViewModels.MainViewModel();
        DataContext = viewModel;

        // Auto-scroll: while the log's scrollbar rests at the bottom, every
        // new entry scrolls the list down so the latest line stays visible.
        // Scrolling up unpins; scrolling back to the bottom re-pins.
        if (viewModel.LogEntries is INotifyCollectionChanged incc)
            incc.CollectionChanged += OnLogEntriesChanged;
        Loaded += (_, _) =>
        {
            _logScrollViewer = FindVisualChild<ScrollViewer>(LogListBox);
            if (_logScrollViewer != null)
                _logScrollViewer.ScrollChanged += OnLogScrollChanged;
        };
    }

    private void OnLogScrollChanged(object sender, ScrollChangedEventArgs e)
    {
        if (_logScrollViewer is null) return;
        // Only user-initiated scrolls (no extent change) update the pin
        // state; auto-scrolls triggered below re-affirm the bottom pin.
        if (e.ExtentHeightChange == 0)
            _logPinnedToBottom =
                _logScrollViewer.VerticalOffset >= _logScrollViewer.ScrollableHeight - 2;
    }

    private void OnLogEntriesChanged(object? sender, NotifyCollectionChangedEventArgs e)
    {
        if (e.Action != NotifyCollectionChangedAction.Add) return;
        if (!_logPinnedToBottom) return;
        // Defer the scroll: touching Items while the CollectionChanged
        // notification is still in flight makes the ItemsControl
        // inconsistent with its items source (the ItemCollection is being
        // regenerated) and throws InvalidOperationException. Scroll after
        // the change has fully settled on the dispatcher.
        Dispatcher.BeginInvoke(System.Windows.Threading.DispatcherPriority.Background, new Action(() =>
        {
            if (!_logPinnedToBottom || LogListBox.Items.Count == 0) return;
            LogListBox.ScrollIntoView(LogListBox.Items[LogListBox.Items.Count - 1]);
        }));
    }

    private static T? FindVisualChild<T>(DependencyObject parent) where T : DependencyObject
    {
        for (var i = 0; i < VisualTreeHelper.GetChildrenCount(parent); i++)
        {
            var child = VisualTreeHelper.GetChild(parent, i);
            if (child is T typed)
                return typed;
            var descendant = FindVisualChild<T>(child);
            if (descendant != null)
                return descendant;
        }
        return null;
    }

    // Right-clicking a log line selects that line first, so the context menu
    // "Copy JSON" action always targets the line under the cursor.
    private void LogListBox_PreviewMouseRightButtonDown(object sender, MouseButtonEventArgs e)
    {
        if (sender is not ListBox listBox) return;
        var item = ItemsControl.ContainerFromElement(
            listBox, e.OriginalSource as DependencyObject) as ListBoxItem;
        item?.Focus();
        if (item != null)
        {
            item.IsSelected = true;
        }
    }
}
