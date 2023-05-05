using Foundation;

namespace Couchbase.Lite.Testing.Maui
{
    public static class ResolvePath
    {
        public static string Resolve(string path, bool unzip)
        {
            var extension = Path.GetExtension(path);
            var withoutExtension = path.Replace(extension, "");
            return NSBundle.MainBundle.PathForResource(withoutExtension, extension.Substring(1));
        }
    }
}
