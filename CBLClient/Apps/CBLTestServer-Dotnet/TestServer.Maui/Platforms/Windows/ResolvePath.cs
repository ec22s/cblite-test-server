namespace Couchbase.Lite.Testing.Maui
{
    public static class ResolvePath
    {
        public static string Resolve(string path, bool unzip)
        {
            return Path.Combine(Windows.ApplicationModel.Package.Current.InstalledLocation.Path, "Assets", path);
        }
    }
}
