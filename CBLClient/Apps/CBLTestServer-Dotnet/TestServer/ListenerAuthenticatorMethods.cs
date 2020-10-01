
using System;
using System.Collections.Generic;
using System.Collections.Specialized;
using System.Net;
using Couchbase.Lite.P2P;
using Couchbase.Lite.Sync;
using Couchbase.Lite.Testing;
using JetBrains.Annotations;

namespace Couchbase.Lite.Testing
{
    public class ListenerAuthenticatorMethods
    {
        public static void Create([NotNull] NameValueCollection args,
                           [NotNull] IReadOnlyDictionary<string, object> postBody,
                           [NotNull] HttpListenerResponse response)
        {
            var username = postBody["username"].ToString();
            var password = postBody["password"].ToString();

            ListenerPasswordAuthenticator authenticator = new ListenerPasswordAuthenticator((sender, user, pass) =>
            {
             
                return user == username && new NetworkCredential(string.Empty, pass).Password == password;
            });
            response.WriteBody(MemoryMap.Store(authenticator));
        }
    }


}
