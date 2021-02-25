package com.couchbase.CouchbaseLiteServ;

import com.couchbase.lite.CouchbaseLiteException;
import com.couchbase.lite.KeyStoreUtils;
import com.couchbase.lite.TLSIdentity;
import com.couchbase.mobiletestkit.javacommon.Context;
import com.couchbase.mobiletestkit.javacommon.util.Log;

import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.net.Inet4Address;
import java.net.InetAddress;
import java.net.NetworkInterface;
import java.security.KeyManagementException;
import java.security.KeyStore;
import java.security.KeyStoreException;
import java.security.NoSuchAlgorithmException;
import java.security.UnrecoverableEntryException;
import java.security.cert.X509Certificate;
import java.security.cert.CertificateException;
import java.security.cert.CertificateFactory;
import java.security.cert.Certificate;
import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.InputStream;
import java.util.ArrayList;
import java.util.UUID;
import java.util.Calendar;
import java.util.Date;
import java.util.HashMap;
import java.util.List;
import java.security.NoSuchProviderException;
import java.security.UnrecoverableEntryException;
import java.security.cert.Certificate;
import java.security.cert.X509Certificate;


import android.util.Base64;

import java.util.Enumeration;

public class TestServerContext implements Context {
    private static final String TAG = "TestServerContext";

    @Override
    public InputStream getAsset(String name) {
        return getClass().getResourceAsStream("/" + name);
    }

    @Override
    public String getPlatform() {
        return "android";
    }

    @Override
    public File getFilesDir() {
        return CouchbaseLiteServ.getAppContext().getFilesDir();
    }

    @Override
    public File getExternalFilesDir(String filetype){
        return CouchbaseLiteServ.getAppContext().getExternalFilesDir(filetype);
    }

    @Override
    public String getLocalIpAddress() {
        String ip = "";
        try {
            for (Enumeration<NetworkInterface> en = NetworkInterface.getNetworkInterfaces(); en.hasMoreElements(); ) {
                NetworkInterface intf = en.nextElement();
                String intf_name = intf.getName();
                Log.d(TAG, "intf_name: " + intf_name);
                for (Enumeration<InetAddress> enumIpAddr = intf.getInetAddresses(); enumIpAddr.hasMoreElements(); ) {
                    InetAddress inetAddress = enumIpAddr.nextElement();
                    if (!inetAddress.isLoopbackAddress() && (intf_name.equals("eth1") || intf_name
                            .equals("wlan0")) && (inetAddress instanceof Inet4Address)) {
                        ip =  inetAddress.getHostAddress();
                        break;
                    }
                }
                if(!ip.isEmpty()) {
                    break;
                }
            }
        }
        catch (java.net.SocketException socketEx) {
            Log.e(TAG, "Socket exception: ", socketEx);
        }
        return ip;
    }

    @Override
    public TLSIdentity getCreateIdentity() {
        Calendar calendar = Calendar.getInstance();
        calendar.add(Calendar.YEAR, 2);
        Date certTime = calendar.getTime();
        HashMap<String, String> X509Attributes = new HashMap<String, String>();
        X509Attributes.put(TLSIdentity.CERT_ATTRIBUTE_COMMON_NAME, "CBL Test");
        X509Attributes.put(TLSIdentity.CERT_ATTRIBUTE_ORGANIZATION, "Couchbase");
        X509Attributes.put(TLSIdentity.CERT_ATTRIBUTE_ORGANIZATION_UNIT, "Mobile");
        X509Attributes.put(TLSIdentity.CERT_ATTRIBUTE_EMAIL_ADDRESS, "lite@couchbase.com");
        String alias = UUID.randomUUID().toString();
        TLSIdentity identity = null;
        try {
            identity = TLSIdentity.createIdentity(true, X509Attributes, certTime, alias);
        } catch (CouchbaseLiteException e) {
            e.printStackTrace();
        }
        System.out.println("CreateIdentity API");
        return identity;
    }

    @Override
    public TLSIdentity getSelfSignedIdentity() {
        TLSIdentity identity = null;
        try {
             InputStream serverCert = this.getCertFile("certs.p12");
             KeyStoreUtils.importEntry("PKCS12",
                        serverCert,
                        "123456".toCharArray(),
                        "testkit",
                        "123456".toCharArray(), "Servercerts");
             identity = TLSIdentity.getIdentity("Servercerts");

            } catch (IOException e) {
                e.printStackTrace();
            } catch (CouchbaseLiteException | KeyStoreException | CertificateException | NoSuchAlgorithmException | UnrecoverableEntryException e) {
                e.printStackTrace();
            }
        return identity;
    }

    @Override
    public List<Certificate> getAuthenticatorCertsList() {
        List<Certificate> certsList = new ArrayList<>();
        try {
             InputStream serverCert;
             serverCert = this.getCertFile("client-ca.der");
             System.out.println(serverCert);
             ByteArrayInputStream derInputStream = new ByteArrayInputStream(toByteArray(serverCert));
             CertificateFactory cf = CertificateFactory.getInstance("X.509");
             X509Certificate cert = (X509Certificate) cf.generateCertificate(derInputStream);
             serverCert.close();
             certsList.add(cert);
            } catch (CertificateException e) {
                e.printStackTrace();
            } catch (IOException e) {
            e.printStackTrace();
        }
        return certsList;
    }

    @Override
    public TLSIdentity getClientCertsIdentity() {
        TLSIdentity identity = null;
        try {
            try (InputStream clientCert = this.getCertFile("client.p12")) {
                try {
                    try {
                        KeyStoreUtils.importEntry("PKCS12",
                                clientCert,
                                "123456".toCharArray(),
                                "testkit",
                                "123456".toCharArray(), "ClientCertsSelfsigned");
                    } catch (IOException e) {
                        e.printStackTrace();
                    } catch (UnrecoverableEntryException e) {
                        e.printStackTrace();
                    }
                    identity = TLSIdentity.getIdentity("ClientCertsSelfsigned");
                } catch (KeyStoreException e) {
                        e.printStackTrace();
                } catch (CertificateException e) {
                        e.printStackTrace();
                } catch (NoSuchAlgorithmException e) {
                        e.printStackTrace();
                } catch (CouchbaseLiteException e) {
                    e.printStackTrace();
                }
            }
        } catch (IOException e) {
            e.printStackTrace();
        }
        return identity;
    }

    @Override
    public String encodeBase64(byte[] hashBytes){
        // load android.util.Base64 in android app
        return Base64.encodeToString(hashBytes, Base64.NO_WRAP);
    }

    private InputStream getCertFile(String name) {
        InputStream is = null;
        try {
            is = getAsset(name);

        } catch (Exception e) {
            e.printStackTrace();
        }
        return is;
    }

    private static byte[] toByteArray(InputStream is) {
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
            io.printStackTrace();
        }

        return bos.toByteArray();
    }
}

