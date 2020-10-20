package com.couchbase.mobiletestkit.javacommon.RequestHandler;

import java.util.Arrays;
import com.couchbase.lite.ListenerPasswordAuthenticatorDelegate;

public class PasswordAuthenticator implements ListenerPasswordAuthenticatorDelegate {
    public boolean authenticate(String username, char[] password) {
        return (username.equals("testkit")) && Arrays.equals(password, "pass".toCharArray());
    }
}
