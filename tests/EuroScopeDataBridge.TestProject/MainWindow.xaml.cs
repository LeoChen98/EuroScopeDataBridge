using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;

namespace EuroScopeDataBridge.TestProject;

public partial class MainWindow : Window
{
    public MainWindow()
    {
        InitializeComponent();
        DataContext = new ViewModels.MainViewModel();
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
