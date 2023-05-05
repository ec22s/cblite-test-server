using System;
using System.IO;
using System.Reflection;
using System.Collections.Generic;
using System.Collections.Specialized;
using System.Net;
using System.Linq;
using System.Security.Cryptography.X509Certificates;
using System.Runtime.Serialization.Formatters.Binary;

using Couchbase.Lite.Sync;
using Couchbase.Lite.Util;

using JetBrains.Annotations;

using Newtonsoft.Json.Linq;

using static Couchbase.Lite.Testing.DatabaseMethods;
using System.Threading;

namespace Couchbase.Lite.Testing
{
    public class ScopeMethods
    {
        public static void defaultScope([NotNull] NameValueCollection args,
                                             [NotNull] IReadOnlyDictionary<string, object> postBody,
                                             [NotNull] HttpListenerResponse response)
        {
            With<Database>(postBody, "database", database =>
            {
                response.WriteBody(MemoryMap.Store(database.GetDefaultScope()));
            });
        }

        public static void scopeName([NotNull] NameValueCollection args,
                                             [NotNull] IReadOnlyDictionary<string, object> postBody,
                                             [NotNull] HttpListenerResponse response)
        {
            With<Scope>(postBody, "scope", scope =>
            {
                response.WriteBody(scope.Name);
            });
        }

        public static void collection([NotNull] NameValueCollection args,
                                             [NotNull] IReadOnlyDictionary<string, object> postBody,
                                             [NotNull] HttpListenerResponse response)
        {
            With<Scope>(postBody, "scope", scope =>
            {
                string collectionName = postBody["collectionName"].ToString();
                response.WriteBody(scope.GetCollection(collectionName));
            });
        }
    }
}