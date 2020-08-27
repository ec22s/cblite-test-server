package com.couchbase.mobiletestkit.javacommon.RequestHandler;

/*
  Created by sridevi.saragadam on 7/9/18.
 */

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.net.URI;
import java.security.cert.Certificate;
import java.security.cert.X509Certificate;
import java.util.ArrayList;
import java.util.Calendar;
import java.util.Date;
import java.util.HashMap;
import java.util.List;

import com.couchbase.lite.ClientCertificateAuthenticator;
import com.couchbase.lite.ConflictResolver;
import com.couchbase.lite.CouchbaseLiteException;
import com.couchbase.lite.ListenerAuthenticator;
import com.couchbase.lite.ListenerCertificateAuthenticator;
import com.couchbase.lite.ListenerCertificateAuthenticatorDelegate;
import com.couchbase.lite.TLSIdentity;
import com.couchbase.lite.URLEndpointListener;
import com.couchbase.lite.URLEndpointListenerConfiguration;
import com.couchbase.lite.internal.KeyStoreManager;
import com.couchbase.mobiletestkit.javacommon.Args;
import com.couchbase.mobiletestkit.javacommon.RequestHandlerDispatcher;
import com.couchbase.mobiletestkit.javacommon.util.Log;
import com.couchbase.lite.Database;
import com.couchbase.lite.MessageEndpoint;
import com.couchbase.lite.MessageEndpointConnection;
import com.couchbase.lite.MessageEndpointDelegate;
import com.couchbase.lite.MessageEndpointListener;
import com.couchbase.lite.MessageEndpointListenerConfiguration;
import com.couchbase.lite.ProtocolType;
import com.couchbase.lite.Replicator;
import com.couchbase.lite.ReplicatorChange;
import com.couchbase.lite.ReplicatorChangeListener;
import com.couchbase.lite.ReplicatorConfiguration;
import com.couchbase.lite.URLEndpoint;

import java.security.KeyManagementException;
import java.security.KeyStore;
import java.security.KeyStoreException;
import java.security.NoSuchAlgorithmException;
import java.security.cert.Certificate;
import java.security.cert.CertificateException;
import java.security.cert.CertificateFactory;


public class PeerToPeerRequestHandler implements MessageEndpointDelegate {
    private static final String TAG = "P2PHANDLER";

    final ReplicatorRequestHandler replicatorRequestHandlerObj = new ReplicatorRequestHandler();

    public void clientStart(Args args) {
        Replicator replicator = args.get("replicator");
        replicator.start();
        Log.i(TAG, "Replication started .... ");
    }

    public Replicator configure(Args args) throws Exception {
        String ipaddress = args.get("host");
        int port = args.get("port");
        Database sourceDb = args.get("database");
        String serverDBName = args.get("serverDBName");
        String replicationType = args.get("replicationType");
        Boolean continuous = args.get("continuous");
        String endPointType = args.get("endPointType");
        List<String> documentIds = args.get("documentIDs");
        Boolean push_filter = args.get("push_filter");
        Boolean pull_filter = args.get("pull_filter");
        String filter_callback_func = args.get("filter_callback_func");
        String conflict_resolver = args.get("conflict_resolver");
        ReplicatorConfiguration config;
        Replicator replicator;
        String tlsAuthType = args.get("tls_auth_type");
        Boolean disableTls = args.get("tls_disable");
        Boolean tls_authenticator = args.get("tls_authenticator");
        Boolean server_verification_mode = args.get("server_verification_mode");
        URI uri;

        if (replicationType == null) {
            replicationType = "push_pull";
        }
        replicationType = replicationType.toLowerCase();
        ReplicatorConfiguration.ReplicatorType replType;
        if (replicationType.equals("push")) {
            replType = ReplicatorConfiguration.ReplicatorType.PUSH;
        } else if (replicationType.equals("pull")) {
            replType = ReplicatorConfiguration.ReplicatorType.PULL;
        } else {
            replType = ReplicatorConfiguration.ReplicatorType.PUSH_AND_PULL;
        }
        Log.i(TAG, "serverDBName is " + serverDBName);
        if (disableTls) {
            uri = new URI("ws://" + ipaddress + ":" + port + "/" + serverDBName);
        } else {
            uri = new URI("wss://" + ipaddress + ":" + port + "/" + serverDBName);
        }
        if (endPointType.equals("URLEndPoint")) {
            URLEndpoint urlEndPoint = new URLEndpoint(uri);
            config = new ReplicatorConfiguration(sourceDb, urlEndPoint);
        } else if (endPointType.equals("MessageEndPoint")) {
            MessageEndpoint messageEndPoint = new MessageEndpoint("p2p", uri, ProtocolType.BYTE_STREAM, this);
            config = new ReplicatorConfiguration(sourceDb, messageEndPoint);
        } else {
            throw new IllegalArgumentException("Incorrect EndPoint type");
        }
        config.setReplicatorType(replType);
        if (continuous != null) {
            config.setContinuous(continuous);
        } else {
            config.setContinuous(false);
        }
        if (documentIds != null) {
            config.setDocumentIDs(documentIds);
        }
        if (push_filter) {
            switch (filter_callback_func) {
                case "boolean":
                    config.setPushFilter(new ReplicatorBooleanFilterCallback());
                    break;
                case "deleted":
                    config.setPushFilter(new ReplicatorDeletedFilterCallback());
                    break;
                case "access_revoked":
                    config.setPushFilter(new ReplicatorAccessRevokedFilterCallback());
                    break;
                default:
                    config.setPushFilter(new DefaultReplicatorFilterCallback());
                    break;
            }
        }
        if (pull_filter) {
            switch (filter_callback_func) {
                case "boolean":
                    config.setPullFilter(new ReplicatorBooleanFilterCallback());
                    break;
                case "deleted":
                    config.setPullFilter(new ReplicatorDeletedFilterCallback());
                    break;
                case "access_revoked":
                    config.setPullFilter(new ReplicatorAccessRevokedFilterCallback());
                    break;
                default:
                    config.setPullFilter(new DefaultReplicatorFilterCallback());
                    break;
            }
        }
        if (args.get("basic_auth") != null) {
            config.setAuthenticator(args.get("basic_auth"));
        }
        if (tlsAuthType == "self_signed") {
            try (InputStream ClientCert = this.getCertFile("certs.p12")) {

                TLSIdentity identity = TLSIdentity.importIdentity("PKCS12",
                        ClientCert,
                        "123".toCharArray(),
                        "serverCerts",
                        null, "serverCerts");
                config.setPinnedServerCertificate(toByteArray(ClientCert));
            } catch (Exception e) {
                e.printStackTrace();
            }
        }

        if (tls_authenticator) {
            try (InputStream ClientCert = this.getCertFile("client.p12")) {
                TLSIdentity identity = TLSIdentity.importIdentity("PKCS12",
                        ClientCert,
                        "123".toCharArray(),
                        "clientCerts",
                        null, "clientCerts");
                ClientCertificateAuthenticator clientCertificateAuthenticator = new ClientCertificateAuthenticator(identity);
                config.setAuthenticator(clientCertificateAuthenticator);
            }
        }

        if (server_verification_mode) {
            config.setAcceptOnlySelfSignedServerCertificate(true);
        }

        switch (conflict_resolver) {
            case "local_wins":
                config.setConflictResolver(new LocalWinsCustomConflictResolver());
                break;
            case "remote_wins":
                config.setConflictResolver(new RemoteWinsCustomConflictResolver());
                break;
            case "null":
                config.setConflictResolver(new NullCustomConflictResolver());
                break;
            case "merge":
                config.setConflictResolver(new MergeCustomConflictResolver());
                break;
            case "incorrect_doc_id":
                config.setConflictResolver(new IncorrectDocIdConflictResolver());
                break;
            case "delayed_local_win":
                config.setConflictResolver(new DelayedLocalWinConflictResolver());
                break;
            case "delete_not_win":
                config.setConflictResolver(new DeleteDocConflictResolver());
                break;
            case "exception_thrown":
                config.setConflictResolver(new ExceptionThrownConflictResolver());
                break;
            default:
                config.setConflictResolver(ConflictResolver.DEFAULT);
                break;
        }
        replicator = new Replicator(config);
        return replicator;
    }

    class MyReplicatorListener implements ReplicatorChangeListener {
        private final List<ReplicatorChange> changes = new ArrayList<>();

        public List<ReplicatorChange> getChanges() {
            return changes;
        }

        @Override
        public void changed(ReplicatorChange change) {
            changes.add(change);
        }
    }

    public URLEndpointListener serverStart(Args args) throws IOException, CouchbaseLiteException {
        int port = args.get("port");
        Database sourceDb = args.get("database");
        URLEndpointListenerConfiguration config = new URLEndpointListenerConfiguration(sourceDb);

        Boolean disableTls = args.get("tls_disable");
        Boolean tls_authenticator = args.get("tls_authenticator");
        String tlsAuthType = args.get("tls_auth_type");

        if (port > 0) {
            port = args.get("port");
            config.setPort(port);
        }
        config.setDisableTls(false);

        if (args.get("basic_auth") != null) {
            ListenerAuthenticator listenerAuthenticator = args.get("basic_auth");
            config.setAuthenticator(listenerAuthenticator);
        }

        if (tlsAuthType == "self_signed_create") {
            Calendar calendar = Calendar.getInstance();
            calendar.add(Calendar.YEAR, 1);
            Date certTime = calendar.getTime();
            HashMap<String, String> X509Attributes = new HashMap<String, String>();
            X509Attributes.put(TLSIdentity.CERT_ATTRIBUTE_COMMON_NAME, "CBL Test");
            X509Attributes.put(TLSIdentity.CERT_ATTRIBUTE_ORGANIZATION, "Couchbase");
            X509Attributes.put(TLSIdentity.CERT_ATTRIBUTE_ORGANIZATION_UNIT, "Mobile");
            X509Attributes.put(TLSIdentity.CERT_ATTRIBUTE_EMAIL_ADDRESS, "lite@couchbase.com");
            TLSIdentity identity = TLSIdentity.createIdentity(true, X509Attributes, certTime, "testkit");
            config.setTlsIdentity(identity);
            config.setAuthenticator(null);
        }

        if (tlsAuthType == "self_signed") {
            try (InputStream ServerCert = this.getCertFile("certs.p12")) {
//                TLSIdentity.deleteIdentity("Servercerts");
                TLSIdentity identity = TLSIdentity.importIdentity("PKCS12",
                        ServerCert,
                        "123".toCharArray(),
                        "ServerCerts",
                        null, "Servercerts");
                config.setTlsIdentity(identity);
                config.setAuthenticator(null);
            }
        }
        if (tls_authenticator) {
            try (InputStream RootCert = this.getCertFile("client-ca.der")) {
                TLSIdentity identity = TLSIdentity.importIdentity("PKCS12",
                        RootCert,
                        "123".toCharArray(),
                        "ServerCerts",
                        null, "Servercerts");
                ListenerCertificateAuthenticator listenerCertificateAuthenticator = new ListenerCertificateAuthenticator(identity.getCerts());
                config.setAuthenticator(listenerCertificateAuthenticator);
            }
        }

        URLEndpointListener p2ptcpListener = new URLEndpointListener(config);
        p2ptcpListener.start();

        System.out.println(p2ptcpListener.getPort());
        return p2ptcpListener;
    }

    public int getListenerPort(Args args) {
        URLEndpointListener p2ptcpListener = args.get("listener");
        return p2ptcpListener.getPort();
    }

    public ReplicatorTcpListener messageEndpointListenerStart(Args args) throws IOException {
        Database sourceDb = args.get("database");
        int port = args.get("port");
        MessageEndpointListener messageEndpointListener =
                new MessageEndpointListener(new MessageEndpointListenerConfiguration(
                        sourceDb,
                        ProtocolType.BYTE_STREAM));
        ReplicatorTcpListener p2ptcpListener = new ReplicatorTcpListener(sourceDb, port);
        p2ptcpListener.start();
        return p2ptcpListener;
    }


    public void serverStop(Args args) {
        String endPointType = args.get("endPointType");
        if (endPointType.equals("MessageEndPoint")) {
            ReplicatorTcpListener p2ptcpListener = args.get("listener");
            p2ptcpListener.stop();
        } else {
            URLEndpointListener p2ptcpListener = args.get("listener");
            p2ptcpListener.stop();
        }

    }

    public MessageEndpointConnection createConnection(MessageEndpoint endpoint) {
        URI url = (URI) endpoint.getTarget();
        return new ReplicatorTcpClientConnection(url);
    }

    public MyDocumentReplicatorListener addReplicatorEventChangeListener(Args args) {
        return replicatorRequestHandlerObj.addReplicatorEventChangeListener(args);
    }

    public void removeReplicatorEventListener(Args args) {
        replicatorRequestHandlerObj.removeReplicatorEventListener(args);
    }

    public int changeListenerChangesCount(Args args) {
        return replicatorRequestHandlerObj.changeListenerChangesCount(args);
    }

    public List<String> replicatorEventGetChanges(Args args) {
        return replicatorRequestHandlerObj.replicatorEventGetChanges(args);
    }

    private InputStream getCertFile(String name) {
        InputStream is = null;
        try {
            is = RequestHandlerDispatcher.context.getAsset(name);
            return is;
        } finally {
            if (is != null) {
                try {
                    is.close();
                } catch (IOException e) {
                }
            }
        }
    }

    public static byte[] toByteArray(InputStream is) {
        ByteArrayOutputStream bos = new ByteArrayOutputStream();
        byte[] b = new byte[1024];

        try {
            int bytesRead = is.read(b);
            while (bytesRead != -1) {
                bos.write(b, 0, bytesRead);
                bytesRead = is.read(b);
            }
        } catch (IOException io) {
            Log.w(TAG, "Got exception " + io.getMessage() + ", Ignoring...");
        }

        return bos.toByteArray();
    }

}
