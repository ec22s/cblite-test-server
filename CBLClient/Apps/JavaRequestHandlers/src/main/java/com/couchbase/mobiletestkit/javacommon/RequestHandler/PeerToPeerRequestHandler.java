package com.couchbase.mobiletestkit.javacommon.RequestHandler;

/*
  Created by sridevi.saragadam on 7/9/18.
 */

import java.io.*;
import java.net.URI;
import java.security.KeyStore;
import java.security.KeyStoreException;
import java.security.NoSuchAlgorithmException;
import java.security.UnrecoverableEntryException;
import java.security.cert.Certificate;
import java.security.cert.CertificateException;
import java.security.cert.CertificateFactory;
import java.util.*;
import java.security.cert.X509Certificate;

import com.couchbase.lite.*;
import com.couchbase.mobiletestkit.javacommon.Args;
import com.couchbase.mobiletestkit.javacommon.RequestHandlerDispatcher;
import com.couchbase.mobiletestkit.javacommon.util.Log;

import static java.security.KeyStore.*;


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
            try (InputStream ServerCert = this.getCertFile("certs.p12")) {
                char[] pass = "123456".toCharArray();
                KeyStore trustStore = KeyStore.getInstance("PKCS12");
                trustStore.load(null,null);
                trustStore.load(ServerCert, pass);
                KeyStore.ProtectionParameter protParam =
                        new KeyStore.PasswordProtection(pass);
                KeyStore.Entry newEntry = trustStore.getEntry("testkit", protParam);
                trustStore.setEntry("Clientcerts", newEntry, protParam);
                TLSIdentity identity = TLSIdentity.getIdentity(trustStore, "Clientcerts", pass);
                if (identity != null) {
                    List<Certificate> certs = identity.getCerts();
                    X509Certificate cert = (X509Certificate) certs.get(0);
                    config.setPinnedServerCertificate(cert.getEncoded());
                    Log.i(TAG, "Pinned the certs ... .... ");
                }
            } catch (UnrecoverableEntryException e) {
                e.printStackTrace();
            }
        }

        if (serverVerificationMode) {
            config.setAcceptOnlySelfSignedServerCertificate(true);
        }
        if (tlsAuthenticator) {
            try (InputStream ClientCert = this.getCertFile("client.p12")) {
                char[] pass = "123456".toCharArray();
                KeyStore trustStore = KeyStore.getInstance("PKCS12");
                trustStore.load(null,null);
                trustStore.load(ClientCert, pass);
                KeyStore.ProtectionParameter protParam =
                        new KeyStore.PasswordProtection(pass);
                KeyStore.Entry newEntry = trustStore.getEntry("testkit", protParam);
                trustStore.setEntry("Clientcerts", newEntry, protParam);
                TLSIdentity identity = TLSIdentity.getIdentity(trustStore, "Clientcerts", pass);
                ClientCertificateAuthenticator clientCertificateAuthenticator = new ClientCertificateAuthenticator(identity);
                config.setAuthenticator(clientCertificateAuthenticator);

            } catch (KeyStoreException e) {
                    e.printStackTrace();
            } catch (CertificateException e) {
                    e.printStackTrace();
            } catch (NoSuchAlgorithmException e) {
                    e.printStackTrace();
            }


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

    public URLEndpointListener serverStart(Args args) throws IOException, CouchbaseLiteException, CouchbaseLiteException, KeyStoreException, CertificateException, NoSuchAlgorithmException {
        int port = args.get("port");
        Boolean disableTls = args.get("tls_disable");
        Database sourceDb = args.get("database");
        String tlsAuthType = args.get("tls_auth_type");
        Boolean tlsAuthenticator = args.get("tls_authenticator");

        URLEndpointListenerConfiguration config = new URLEndpointListenerConfiguration(sourceDb);
        if (port > 0) {
            port = args.get("port");
            config.setPort(port);
        }
        config.setDisableTls(disableTls);

        if (tlsAuthType.equals("self_signed_create")) {
            final String EXTERNAL_KEY_STORE_TYPE = "PKCS12";
            Calendar calendar = Calendar.getInstance();
            calendar.add(Calendar.YEAR, 1);
            Date certTime = calendar.getTime();
            HashMap<String, String> X509Attributes = new HashMap<String, String>();
            X509Attributes.put(TLSIdentity.CERT_ATTRIBUTE_COMMON_NAME, "CBL Test");
            X509Attributes.put(TLSIdentity.CERT_ATTRIBUTE_ORGANIZATION, "Couchbase");
            X509Attributes.put(TLSIdentity.CERT_ATTRIBUTE_ORGANIZATION_UNIT, "Mobile");
            X509Attributes.put(TLSIdentity.CERT_ATTRIBUTE_EMAIL_ADDRESS, "lite@couchbase.com");

            KeyStore externalStore = KeyStore.getInstance(EXTERNAL_KEY_STORE_TYPE);
            externalStore.load(null, null);
            String alias = UUID.randomUUID().toString();

            TLSIdentity identity = TLSIdentity.createIdentity(true,
                    X509Attributes,
                    certTime,
                    externalStore,
                    alias,
                    "pass".toCharArray());
            config.setTlsIdentity(identity);

        } else if (tlsAuthType.equals("self_signed")) {
            try (InputStream ServerCert = this.getCertFile("certs.p12")) {
                char[] pass = "123456".toCharArray();
                KeyStore trustStore = KeyStore.getInstance("PKCS12");
                trustStore.load(null,null);
                trustStore.load(ServerCert, pass);
                KeyStore.ProtectionParameter protParam =
                        new KeyStore.PasswordProtection(pass);
                KeyStore.Entry newEntry = trustStore.getEntry("testkit", protParam);
                trustStore.setEntry("Servercerts", newEntry, protParam);
                TLSIdentity identity = TLSIdentity.getIdentity(trustStore, "Servercerts", pass);
                config.setTlsIdentity(identity);
            } catch (UnrecoverableEntryException e) {
                e.printStackTrace();
            }
        }
        if (tlsAuthenticator) {
            try (InputStream ClientCert = this.getCertFile("client-ca.der")) {
                try {
                    CertificateFactory cf = CertificateFactory.getInstance("X.509");
                    Certificate cert;
                    List<Certificate> certsList = new ArrayList<>();
                    cert = cf.generateCertificate(new BufferedInputStream(ClientCert));
                    certsList.add(cert);
                    ListenerCertificateAuthenticator listenerCertificateAuthenticator = new ListenerCertificateAuthenticator(certsList);
                    config.setAuthenticator(listenerCertificateAuthenticator);
                } catch (CertificateException e) {
                    e.printStackTrace();
                }

            }
        }


        if (args.get("basic_auth") != null) {
            ListenerAuthenticator listenerAuthenticator = args.get("basic_auth");
            System.out.println(listenerAuthenticator);
            config.setAuthenticator(listenerAuthenticator);
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

    public void serverStop(Args args) {
        String endPointType = args.get("endPointType");
        if (endPointType.equals("MessageEndPoint")) {
            ReplicatorTcpListener tcpListener = args.get("listener");
            tcpListener.stop();
        } else {
            URLEndpointListener urlListener = args.get("listener");
            urlListener.stop();
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

    private InputStream getCertFile(String fileName) {
        InputStream is = null;
        try {
            is = RequestHandlerDispatcher.context.getAsset(fileName);
            return is;
        } catch (Exception e) {
            e.printStackTrace();
        }
        return is;
    }

}
