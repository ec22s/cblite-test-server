using System;
using System.Collections.Generic;
using System.Collections.Specialized;
using System.Linq;
using System.Net;
using Couchbase.Lite.P2P;
using System.Threading.Tasks;

using Couchbase.Lite;
using static Couchbase.Lite.Testing.P2PMethods;

using JetBrains.Annotations;

using Newtonsoft.Json.Linq;
using Couchbase.Lite.Sync;
using System.Security.Cryptography.X509Certificates;
using System.IO;
using System.Reflection;

namespace Couchbase.Lite.Testing
{
    public class P2PMethods
    {
        static private MessageEndpointListener _messageEndpointListener;
        static private ReplicatorTcpListener _broadcaster;
        private static X509Store _store;
        private const string ServerCertLabel = "CBL-Server-Cert";
        private const string ClientCertLabel = "CBL-Client-Cert";
        private const string V = "tls_disable";

        public static void Message_Endpoint_Listener_Start([NotNull] NameValueCollection args,
                                [NotNull] IReadOnlyDictionary<string, object> postBody,
                                [NotNull] HttpListenerResponse response)
        {
            ResetStatus();
            Database db = MemoryMap.Get<Database>(postBody["database"].ToString());
            int port = (int)postBody["port"];
            _messageEndpointListener = new MessageEndpointListener(new MessageEndpointListenerConfiguration(db, ProtocolType.ByteStream));
            _broadcaster = new ReplicatorTcpListener(_messageEndpointListener, port);
            _broadcaster.Start();
            AddStatus("Start waiting for connection..");
            response.WriteBody(MemoryMap.Store(_broadcaster));
        }

        public static void Start_Server([NotNull] NameValueCollection args,
                                [NotNull] IReadOnlyDictionary<string, object> postBody,
                                [NotNull] HttpListenerResponse response)
        {
            ResetStatus();
            
            Database db = MemoryMap.Get<Database>(postBody["database"].ToString());
            int port = (int)postBody["port"];
            URLEndpointListenerConfiguration urlEndpointListenerConfig = new URLEndpointListenerConfiguration(db);
            string tlsAuthType = postBody["tls_auth_type"].ToString();
            Boolean disableTls = Convert.ToBoolean(postBody[V].ToString());
            Boolean tls_authenticator = Convert.ToBoolean(postBody["tls_authenticator"].ToString());
            Boolean enableDeltaSync = Convert.ToBoolean(postBody["enable_delta_sync"].ToString());

            AddStatus("URL Start SERVER");
            if (port > 0)
            {
                urlEndpointListenerConfig.Port = (ushort)port;
            }

            if (postBody.ContainsKey("basic_auth"))
            {
               urlEndpointListenerConfig.Authenticator = MemoryMap.Get<ListenerPasswordAuthenticator>(postBody["basic_auth"].ToString());
            }
    
            urlEndpointListenerConfig.DisableTLS = disableTls;
            if (tlsAuthType == "self_signed")
            {
                Stream stream;
                _store = new X509Store(StoreName.My);
                byte[] cert = null;
                TLSIdentity.DeleteIdentity(_store, ServerCertLabel, null);
                string certLocation = TestServer.FilePathResolver("certs/certs.p12", false);
                byte[] certsData = File.ReadAllBytes(certLocation);
                TLSIdentity identity = TLSIdentity.ImportIdentity(_store, certsData, "123", ServerCertLabel, null);
                urlEndpointListenerConfig.TlsIdentity = identity;
                

            } else if (tlsAuthType == "self_signed_create")
            {
                AddStatus("CreateIdentity");
                _store = new X509Store(StoreName.My);
                TLSIdentity.DeleteIdentity(_store, ServerCertLabel, null);
                TLSIdentity identity = TLSIdentity.CreateIdentity(false,
                    new Dictionary<string, string>() { { Certificate.CommonNameAttribute, ServerCertLabel } },
                    null,
                    _store,
                    ServerCertLabel,
                    null);

                urlEndpointListenerConfig.TlsIdentity = identity;
            }
            if (tls_authenticator) {
                _store = new X509Store(StoreName.My);
                TLSIdentity.DeleteIdentity(_store, ServerCertLabel, null);
                string certLocation = TestServer.FilePathResolver("certs/client-ca.der", false);
                byte[] caData = File.ReadAllBytes(certLocation);

                var rootCert = new X509Certificate2(caData);
                var auth = new ListenerCertificateAuthenticator(new X509Certificate2Collection(rootCert));
                urlEndpointListenerConfig.Authenticator = auth;
            }

            urlEndpointListenerConfig.EnableDeltaSync = enableDeltaSync;

            URLEndpointListener urlEndpointListener = new URLEndpointListener(urlEndpointListenerConfig);
            urlEndpointListener.Start();
            AddStatus("Started the url listener and waiting for connection..On the below port");
            Console.WriteLine(urlEndpointListener.Port);
            response.WriteBody(MemoryMap.Store(urlEndpointListener));
        }

        public static void Stop_Server([NotNull] NameValueCollection args,
                                [NotNull] IReadOnlyDictionary<string, object> postBody,
                                [NotNull] HttpListenerResponse response)
        {
            string listenerType = postBody["endPointType"].ToString();
 
            if (listenerType == "MessageEndPoint")
            {
                Console.WriteLine("Stopping Message Endpoint");
                ReplicatorTcpListener _listener = MemoryMap.Get<ReplicatorTcpListener>(postBody["listener"].ToString());
                _listener.Stop();
            }
            else {
                URLEndpointListener _listener = MemoryMap.Get<URLEndpointListener>(postBody["listener"].ToString());
                _listener.Stop();
            }
                
            AddStatus("Stopping the server..");
            response.WriteEmptyBody();
        }

        public static void Get_Listener_Port([NotNull] NameValueCollection args,
                                [NotNull] IReadOnlyDictionary<string, object> postBody,
                                [NotNull] HttpListenerResponse response)
        {
            URLEndpointListener _urlEndpointListener = MemoryMap.Get<URLEndpointListener>(postBody["listener"].ToString());

            ushort port = _urlEndpointListener.Port;
            response.WriteBody((int)port);
        }

        public static void Configure([NotNull] NameValueCollection args,
                                 [NotNull] IReadOnlyDictionary<string, object> postBody,
                                 [NotNull] HttpListenerResponse response)
        {
            ResetStatus();
            Database db = MemoryMap.Get<Database>(postBody["database"].ToString());
            int port = (int)postBody["port"];
            string targetIP = postBody["host"].ToString();
            string remote_DBName = postBody["serverDBName"].ToString();
            string replicationType = postBody["replicationType"].ToString();
            string endPointType = postBody["endPointType"].ToString();
            string filter_callback_func = postBody["filter_callback_func"].ToString();
            Boolean push_filter = Convert.ToBoolean(postBody["push_filter"].ToString());
            Boolean pull_filter = Convert.ToBoolean(postBody["pull_filter"].ToString());
            Boolean tls_disable = Convert.ToBoolean(postBody["tls_disable"].ToString());
            ReplicatorConfiguration config = null;
            string tlsAuthType = postBody["tls_auth_type"].ToString();
            Boolean tls_authenticator = Convert.ToBoolean(postBody["tls_authenticator"].ToString());
            Boolean server_verification_mode = Convert.ToBoolean(postBody["server_verification_mode"].ToString());

            Uri host;
            if (tls_disable)
            {
                host = new Uri("ws://" + targetIP + ":" + port);
            } else {
                host = new Uri("wss://" + targetIP + ":" + port);
            }
            var dbUrl = new Uri(host, remote_DBName);
            AddStatus("Connecting " + host + "...");
            if (endPointType == "URLEndPoint")
            {
                var _endpoint = new URLEndpoint(dbUrl);
                config = new ReplicatorConfiguration(db, _endpoint);
                
            }
            else {
                TcpMessageEndpointDelegate endpointDelegate = new TcpMessageEndpointDelegate();
                var _endpoint = new MessageEndpoint(dbUrl.AbsoluteUri, dbUrl, ProtocolType.ByteStream, endpointDelegate);
                config = new ReplicatorConfiguration(db, _endpoint);
            }

            var replicatorType = replicationType.ToLower();
            if (replicatorType == "push")
            {
                    config.ReplicatorType = ReplicatorType.Push;
            }
            else if (replicatorType == "pull")
            {
                    config.ReplicatorType = ReplicatorType.Pull;
            }
            else
            {
                    config.ReplicatorType = ReplicatorType.PushAndPull;
            }
            if (postBody.ContainsKey("continuous"))
            {
                config.Continuous = Convert.ToBoolean(postBody["continuous"]);
            }
            if (postBody.ContainsKey("documentIDs"))
            {
                List<object> documentIDs = (List<object>)postBody["documentIDs"];
                config.DocumentIDs = documentIDs.Cast<string>().ToList();
            }
            if (postBody["push_filter"].Equals(true))
            {
                if (filter_callback_func == "boolean")
                {
                    config.PushFilter = _replicator_boolean_filter_callback;
                }
                else if (filter_callback_func == "deleted")
                {
                    config.PushFilter = _replicator_deleted_filter_callback;
                }
                else if (filter_callback_func == "access_revoked")
                {
                    config.PushFilter = _replicator_access_revoked_filter_callback;
                }
                else
                {
                    config.PushFilter = _default_replicator_filter_callback;
                }
            }

            if (postBody["pull_filter"].Equals(true))
            {
                if (filter_callback_func == "boolean")
                {
                    config.PullFilter = _replicator_boolean_filter_callback;
                }
                else if (filter_callback_func == "deleted")
                {
                    config.PullFilter = _replicator_deleted_filter_callback;
                }
                else if (filter_callback_func == "access_revoked")
                {
                    config.PullFilter = _replicator_access_revoked_filter_callback;
                }
                else
                {
                    config.PullFilter = _default_replicator_filter_callback;
                }

            }
            if (postBody.ContainsKey("basic_auth"))
            {
                config.Authenticator = MemoryMap.Get<Authenticator>(postBody["basic_auth"].ToString());
            }
            if (tlsAuthType == "self_signed")
            {
                _store = new X509Store(StoreName.My);
                TLSIdentity.DeleteIdentity(_store, ClientCertLabel, null);
                string certLocation = TestServer.FilePathResolver("certs/certs.p12", false);
                byte[] certsData = File.ReadAllBytes(certLocation);
                TLSIdentity identity = TLSIdentity.ImportIdentity(_store, certsData, "123", ClientCertLabel, null);
                config.PinnedServerCertificate = identity.Certs[0];
            }
            if (tls_authenticator) {
                _store = new X509Store(StoreName.My);
                TLSIdentity.DeleteIdentity(_store, ClientCertLabel, null);
                string certLocation = TestServer.FilePathResolver("certs/client.p12", false);
                byte[] certsData = File.ReadAllBytes(certLocation);
                var identity = TLSIdentity.ImportIdentity(_store, certsData, "123", ClientCertLabel, null);
                config.Authenticator = new ClientCertificateAuthenticator(identity);

            } if (server_verification_mode) {
                config.AcceptOnlySelfSignedServerCertificate = true;
                
                AddStatus(config.ToString().ToString());
                System.Console.WriteLine(config.ToString());
            }

            switch (postBody["conflict_resolver"].ToString())
            {
                case "local_wins":
                    config.ConflictResolver = new LocalWinsCustomConflictResolver();
                    break;
                case "remote_wins":
                    config.ConflictResolver = new RemoteWinsCustomConflictResolver();
                    break;
                case "null":
                    config.ConflictResolver = new NullCustomConflictResolver();
                    break;
                case "merge":
                    config.ConflictResolver = new MergeCustomConflictResolver();
                    break;
                case "incorrect_doc_id":
                    config.ConflictResolver = new IncorrectDocIdConflictResolver();
                    break;
                case "delayed_local_win":
                    config.ConflictResolver = new DelayedLocalWinConflictResolver();
                    break;
                case "delete_not_win":
                    config.ConflictResolver = new DeleteDocConflictResolver();
                    break;
                case "exception_thrown":
                    config.ConflictResolver = new ExceptionThrownConflictResolver();
                    break;
                default:
                    config.ConflictResolver = ConflictResolver.Default;
                    break;
            }

            if (postBody.ContainsKey("heartbeat"))
            {
                String heartbeat = postBody["heartbeat"].ToString();
                if (String.IsNullOrEmpty(heartbeat.Trim()))
                {
                    config.Heartbeat = new System.TimeSpan(long.Parse(heartbeat));
                }
            }

            if (postBody.ContainsKey("max_retries"))
            {
                String maxRetries = postBody["max_retries"].ToString();
                if (String.IsNullOrEmpty(maxRetries.Trim()))
                {
                    config.MaxAttempts = int.Parse(maxRetries);
                }
            }

            if (postBody.ContainsKey("max_timeout"))
            {
                String maxRetryWaitTime = postBody["max_timeout"].ToString();
                if (String.IsNullOrEmpty(maxRetryWaitTime.Trim()))
                {
                    config.MaxAttemptsWaitTime = new System.TimeSpan(long.Parse(maxRetryWaitTime));
                }
            }

            Replicator replicator = new Replicator(config);

            response.WriteBody(MemoryMap.Store(replicator));

        }

        private static bool _replicator_boolean_filter_callback(Document document, DocumentFlags flags)
        {
            if (document.Contains("new_field_1"))
            {
                return document.GetBoolean("new_field_1");
            }
            return true;
        }

        private static bool _default_replicator_filter_callback(Document document, DocumentFlags flags)
        {
            return true;
        }

        private static bool _replicator_deleted_filter_callback(Document document, DocumentFlags flags)
        {
            return !flags.HasFlag(DocumentFlags.Deleted);
        }

        private static bool _replicator_access_revoked_filter_callback(Document document, DocumentFlags flags)
        {
            return !flags.HasFlag(DocumentFlags.AccessRemoved);
        }


        public static void Start_Client([NotNull] NameValueCollection args,
                                 [NotNull] IReadOnlyDictionary<string, object> postBody,
                                 [NotNull] HttpListenerResponse response)
        {
            Replicator replicator = MemoryMap.Get<Replicator>(postBody["replicator"].ToString());
            replicator.Start();
            response.WriteEmptyBody();
        }
            static private void ResetStatus()
        {
            // Console.Clear();
            Console.WriteLine("Status is getting reset");
        }

        static public void AddStatus(string newStatus)
        {
            Console.WriteLine(newStatus);
        }
    }
}
