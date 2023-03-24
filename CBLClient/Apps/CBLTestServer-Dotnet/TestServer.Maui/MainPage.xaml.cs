namespace Couchbase.Lite.Testing.Maui;

public partial class MainPage : ContentPage
{
    public MainPage()
    {
        InitializeComponent();
        TestServer.FilePathResolver = ResolvePath.Resolve;
        var listener = new TestServer();
        listener.Start();
        Label.Text = "CBLTestServer-Maui - listening!";
    }
}

