﻿using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices.WindowsRuntime;
using Windows.Foundation;
using Windows.Foundation.Collections;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Controls.Primitives;
using Windows.UI.Xaml.Data;
using Windows.UI.Xaml.Input;
using Windows.UI.Xaml.Media;
using Windows.UI.Xaml.Navigation;

// The Blank Page item template is documented at https://go.microsoft.com/fwlink/?LinkId=402352&clcid=0x409

namespace Couchbase.Lite.Testing.UWP
{
    /// <summary>
    /// An empty page that can be used on its own or navigated to within a Frame.
    /// </summary>
    public sealed partial class MainPage : Page
    {
        public MainPage()
        {
            this.InitializeComponent();
			TestServer.FilePathResolver = ResolvePath;
            var listener = new TestServer();
            listener.Start();
            Label.Text = "CBLTestServer-NetUWP - listening!";
        }

		private string ResolvePath(string path, bool unzip)
		{
			return Path.Combine(Windows.ApplicationModel.Package.Current.InstalledLocation.Path, "Assets", path);
		}
    }
}
