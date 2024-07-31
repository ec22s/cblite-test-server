package com.couchbase.mobiletestkit.javatestserver;
import com.couchbase.lite.CouchbaseLiteException;
import com.couchbase.mobiletestkit.javacommon.Context;
import com.couchbase.mobiletestkit.javacommon.util.Log;

import java.net.*;
import java.security.UnrecoverableEntryException;
import java.util.Collections;
import java.util.Enumeration;
import java.util.Base64;

import com.couchbase.lite.TLSIdentity;

import java.security.cert.Certificate;
import java.util.ArrayList;
import java.util.List;
import java.util.UUID;
import java.util.Calendar;
import java.util.Date;
import java.util.HashMap;
import java.io.*;

import java.security.KeyStore;
import java.security.KeyStoreException;
import java.security.NoSuchAlgorithmException;
import java.security.cert.CertificateException;
import java.security.cert.CertificateFactory;


public class TestServerContext implements Context {
    private final String TAG = "JavaContext";
    private File directory;

    public TestServerContext(File directory) {
        this.directory = directory;
    }

    @Override
    public File getFilesDir() {
        return this.directory;
    }

    @Override
    public File getExternalFilesDir(String filetype) {
        File externalFilesDir = new File(this.directory.getAbsolutePath(), filetype);
        externalFilesDir.mkdir();

        return externalFilesDir;
    }


    @Override
    public InputStream getAsset(String name) {
        return TestServerMain.class.getResourceAsStream("/" + name);
    }

    @Override
    public File getAssetAsFile(String name) {
        URL resource  = TestServerMain.class.getResource("/" + name);
        try {
            return new File(resource.toURI());
        }
        catch (URISyntaxException e) {
            e.printStackTrace();
            return null;
        }
    }

    @Override
    public String getPlatform() {
        return "java";
    }

    @Override
    public String getLocalIpAddress() {
        String ip = null;

        NetworkInterface en0 = null;
        NetworkInterface eth1 = null;
        try {
            Enumeration<NetworkInterface> nets = NetworkInterface.getNetworkInterfaces();
            for (NetworkInterface netint : Collections.list(nets)) {
                if (en0 == null && netint.getName().equals("en0")) {
                    en0 = netint;
                    continue;
                }
                if (eth1 == null && netint.getName().equals("eth1")) {
                    eth1 = netint;
                    continue;
                }
            }

            if (eth1 != null) {
                ip = getIpAddressByInterface(eth1);
                if (!ip.isEmpty()) {
                    return ip;
                }
            }

            if (en0 != null) {
                ip = getIpAddressByInterface(en0);
                if (!ip.isEmpty()) {
                    return ip;
                }
            }

            if (ip == null || ip.isEmpty()) {
                ip = InetAddress.getLocalHost().getHostAddress();
            }
        } catch (SocketException socketEx) {
            Log.e(TAG, "SocketException: ", socketEx);
        } catch (UnknownHostException ex) {
            Log.e(TAG, "UnknownHostException: ", ex);
        }

        return ip;
    }

    @Override
    public String encodeBase64(byte[] hashBytes) {
        // load java.util.Base64 in java standalone app
        return Base64.getEncoder().encodeToString(hashBytes);
    }

    @Override
    public byte[] decodeBase64(String encodedBytes) {
        // load java.util.Base64 in java standalone app
        return Base64.getDecoder().decode(encodedBytes);
    }

    @Override
    public TLSIdentity getCreateIdentity() {
        final String EXTERNAL_KEY_STORE_TYPE = "PKCS12";
        Calendar calendar = Calendar.getInstance();
        calendar.add(Calendar.YEAR, 1);
        Date certTime = calendar.getTime();
        HashMap<String, String> X509Attributes = new HashMap<String, String>();
        X509Attributes.put(TLSIdentity.CERT_ATTRIBUTE_COMMON_NAME, "CBL Test");
        X509Attributes.put(TLSIdentity.CERT_ATTRIBUTE_ORGANIZATION, "Couchbase");
        X509Attributes.put(TLSIdentity.CERT_ATTRIBUTE_ORGANIZATION_UNIT, "Mobile");
        X509Attributes.put(TLSIdentity.CERT_ATTRIBUTE_EMAIL_ADDRESS, "lite@couchbase.com");
        KeyStore externalStore = null;
        try {
            externalStore = KeyStore.getInstance(EXTERNAL_KEY_STORE_TYPE);
        } catch (KeyStoreException e) {
            e.printStackTrace();
        }
        try {
            externalStore.load(null, null);
        } catch (IOException e) {
            e.printStackTrace();
        } catch (NoSuchAlgorithmException e) {
            e.printStackTrace();
        } catch (CertificateException e) {
            e.printStackTrace();
        }
        String alias = UUID.randomUUID().toString();
        TLSIdentity identity = null;
        try {
            identity = TLSIdentity.createIdentity(true,
                    X509Attributes,
                    certTime,
                    externalStore,
                    alias,
                    "pass".toCharArray()
            );
        } catch (CouchbaseLiteException e) {
            e.printStackTrace();
        }
        return identity;
    }

    @Override
    public TLSIdentity getSelfSignedIdentity() {
        TLSIdentity identity = null;
        try (InputStream ServerCert = this.getCertFile("certs.p12")) {
            char[] pass = "123456".toCharArray();
            KeyStore trustStore = null;
            try {
                trustStore = KeyStore.getInstance("PKCS12");
                trustStore.load(null, null);
                trustStore.load(ServerCert, pass);

                KeyStore.ProtectionParameter protParam = new KeyStore.PasswordProtection(pass);
                KeyStore.Entry newEntry = null;
                newEntry = trustStore.getEntry("testkit", protParam);
                trustStore.setEntry("Servercerts", newEntry, protParam);
                identity = TLSIdentity.getIdentity(trustStore, "Servercerts", pass);
            } catch (CouchbaseLiteException e) {
                e.printStackTrace();
            } catch (NoSuchAlgorithmException e) {
                e.printStackTrace();
            } catch (CertificateException e) {
                e.printStackTrace();
            } catch (UnrecoverableEntryException e) {
                e.printStackTrace();
            } catch (KeyStoreException e) {
                e.printStackTrace();
            }
        } catch (IOException e) {
            e.printStackTrace();
        }
        return identity;
    }

    @Override
    public List<Certificate> getAuthenticatorCertsList() {
        List<Certificate> certsList = new ArrayList<>();
        try (InputStream ClientCert = this.getCertFile("client-ca.der")) {
            try {
                CertificateFactory cf = CertificateFactory.getInstance("X.509");
                Certificate cert;
                cert = cf.generateCertificate(new BufferedInputStream(ClientCert));
                certsList.add(cert);
            } catch (CertificateException e) {
                e.printStackTrace();
            }
        } catch (IOException e) {
            e.printStackTrace();
        }
        return certsList;
    }

    @Override
    public TLSIdentity getClientCertsIdentity() {
        TLSIdentity identity = null;
        try (InputStream ClientCert = this.getCertFile("client.p12")) {
            char[] pass = "123456".toCharArray();
            KeyStore trustStore = KeyStore.getInstance("PKCS12");
            trustStore.load(null, null);
            trustStore.load(ClientCert, pass);
            KeyStore.ProtectionParameter protParam = new KeyStore.PasswordProtection(pass);
            KeyStore.Entry newEntry = trustStore.getEntry("testkit", protParam);
            trustStore.setEntry("Clientcerts", newEntry, protParam);
            identity = TLSIdentity.getIdentity(trustStore, "Clientcerts", pass);

        } catch (KeyStoreException e) {
            e.printStackTrace();
        } catch (CertificateException | CouchbaseLiteException e) {
            e.printStackTrace();
        } catch (NoSuchAlgorithmException e) {
            e.printStackTrace();
        } catch (IOException e) {
            e.printStackTrace();
        } catch (UnrecoverableEntryException e) {
            e.printStackTrace();
        }
        return identity;
    }

    private InputStream getCertFile(String fileName) {
        InputStream is = null;
        try {
            is = getAsset(fileName);
            return is;
        } catch (Exception e) {
            e.printStackTrace();
        }
        return is;
    }

    private String getIpAddressByInterface(NetworkInterface networkInterface) {
        String ip = "";

        Enumeration<InetAddress> inetAddresses = networkInterface.getInetAddresses();
        for (InetAddress address : Collections.list(inetAddresses)) {
            if (address instanceof Inet4Address) {
                // currently support ipv4 address
                ip = address.getHostAddress();
                return ip;
            } else if (address instanceof Inet6Address) {
                // do nothing for ipv6 address now, may need for future
            }
        }
        return ip;
    }
}