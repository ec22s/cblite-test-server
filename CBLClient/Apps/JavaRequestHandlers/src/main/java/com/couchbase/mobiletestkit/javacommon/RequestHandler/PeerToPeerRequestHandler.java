package com.couchbase.mobiletestkit.javacommon.RequestHandler;

/*
  Created by sridevi.saragadam on 7/9/18.
 */

import java.io.IOException;
import java.net.URI;
import java.security.cert.Certificate;
import java.security.cert.X509Certificate;
import java.util.ArrayList;
import java.util.List;
import com.couchbase.lite.ClientCertificateAuthenticator;
import com.couchbase.lite.ConflictResolver;
import com.couchbase.lite.CouchbaseLiteException;
import com.couchbase.lite.ListenerAuthenticator;
import com.couchbase.lite.ListenerCertificateAuthenticator;
import com.couchbase.lite.TLSIdentity;
import com.couchbase.lite.URLEndpointListener;
import com.couchbase.lite.URLEndpointListenerConfiguration;
import com.couchbase.lite.*;
import com.couchbase.mobiletestkit.javacommon.Args;
import com.couchbase.mobiletestkit.javacommon.RequestHandlerDispatcher;
import com.couchbase.mobiletestkit.javacommon.util.Log;


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
        Boolean disableTls = args.get("tls_disable");
        String tlsAuthType = args.get("tls_auth_type");
        Boolean serverVerificationMode = args.get("server_verification_mode");
        Boolean tlsAuthenticator = args.get("tls_authenticator");
        String heartbeat = args.get("heartbeat");
        String maxRetries = args.get("max_retries");
        String maxTimeout = args.get("max_timeout");

        ReplicatorConfiguration config;
        Replicator replicator;
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
        if (heartbeat != null && !heartbeat.trim().isEmpty()){
            config.setHeartbeat(Integer.parseInt(heartbeat));
        }

        if (maxRetries != null && !maxRetries.trim().isEmpty()) {
            config.setMaxAttempts(Integer.parseInt(maxRetries));
        }

        if (maxTimeout != null && !maxTimeout.trim().isEmpty()) {
            config.setMaxAttemptWaitTime(Integer.parseInt(maxTimeout));
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

        if (tlsAuthType.equals("self_signed")) {
            TLSIdentity tlsIdentity = RequestHandlerDispatcher.context.getSelfSignedIdentity();
            if (tlsIdentity != null) {
                List<Certificate> certs = tlsIdentity.getCerts();
                X509Certificate cert = (X509Certificate) certs.get(0);
                config.setPinnedServerCertificate(cert.getEncoded());
                Log.i(TAG, "Pinned the certs ... .... ");
            }
        }

        if (tlsAuthenticator) {
            TLSIdentity identity = RequestHandlerDispatcher.context.getClientCertsIdentity();
            ClientCertificateAuthenticator clientCertificateAuthenticator = new ClientCertificateAuthenticator(identity);
            config.setAuthenticator(clientCertificateAuthenticator);
        }

        if (serverVerificationMode) {
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
        Boolean tlsAuthenticator = args.get("tls_authenticator");
        String tlsAuthType = args.get("tls_auth_type");
        System.out.println(tlsAuthType);

        if (port > 0) {
            port = args.get("port");
            config.setPort(port);
        }
        config.setDisableTls(disableTls);

        if (args.get("basic_auth") != null) {
            ListenerAuthenticator listenerAuthenticator = args.get("basic_auth");
            config.setAuthenticator(listenerAuthenticator);
        }

        if (tlsAuthType.equals("self_signed_create")) {
            TLSIdentity identity = RequestHandlerDispatcher.context.getCreateIdentity();
            config.setTlsIdentity(identity);
        }

        if (tlsAuthType.equals("self_signed")) {
            TLSIdentity identity = RequestHandlerDispatcher.context.getSelfSignedIdentity();
            config.setTlsIdentity(identity);
            Log.e(TAG,"ServerSide setting the identity");
        }
        if (tlsAuthenticator) {
            List<Certificate> certsList = RequestHandlerDispatcher.context.getAuthenticatorCertsList();
            ListenerCertificateAuthenticator listenerCertificateAuthenticator = new ListenerCertificateAuthenticator(certsList);
            config.setAuthenticator(listenerCertificateAuthenticator);
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
}
