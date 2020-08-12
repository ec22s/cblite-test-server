package com.couchbase.mobiletestkit.javacommon.RequestHandler;

import android.support.annotation.NonNull;

import com.couchbase.lite.ListenerPasswordAuthenticatorDelegate;

public class PasswordAuthenticator implements ListenerPasswordAuthenticatorDelegate {
    public boolean authenticate(@NonNull String username, @NonNull char[] password) {
        return (username.equals("testkit")) && password.equals("pass");
    }
}
