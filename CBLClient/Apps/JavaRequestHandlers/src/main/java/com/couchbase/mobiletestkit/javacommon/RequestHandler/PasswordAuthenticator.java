package com.couchbase.mobiletestkit.javacommon.RequestHandler;

import android.support.annotation.NonNull;
import java.util.Arrays;

import com.couchbase.lite.ListenerPasswordAuthenticatorDelegate;

public class PasswordAuthenticator implements ListenerPasswordAuthenticatorDelegate {
    public boolean authenticate(@NonNull String username, @NonNull char[] password) {
        return (username.equals("testkit")) && Arrays.equals(password, "pass".toCharArray());
    }
}
