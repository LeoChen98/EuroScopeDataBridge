using System.Windows;

namespace EuroScopeDataBridge.TestProject;

public partial class MainWindow : Window
{
    public MainWindow()
    {
        InitializeComponent();
        DataContext = new ViewModels.MainViewModel();
    }
}
