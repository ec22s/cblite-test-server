package com.couchbase.mobiletestkit.javacommon;

import com.couchbase.lite.TLSIdentity;

import java.io.File;
import java.io.InputStream;
import java.security.cert.Certificate;
import java.util.List;

public interface Context {
    File getFilesDir();
    File getExternalFilesDir(String filetype);
    InputStream getAsset(String name);
    String getPlatform();
    String getLocalIpAddress();
    TLSIdentity getCreateIdentity();
    TLSIdentity getSelfSignedIdentity();
    List<Certificate> getAuthenticatorCertsList();
    TLSIdentity getClientCertsIdentity();
    /*
     * the customEncode method allows custom Base64 package
     * is loaded by platform specific:
     * java.util.Base64 in java standalone and web application
     * android.util.Base64 in android apps
     */
    String encodeBase64(byte[] hashBytes);
    byte[] decodeBase64(String encodedBytes);
}
