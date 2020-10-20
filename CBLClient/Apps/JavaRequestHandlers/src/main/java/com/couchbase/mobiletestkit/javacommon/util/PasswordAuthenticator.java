package com.couchbase.mobiletestkit.javacommon.util;

import com.couchbase.lite.ListenerPasswordAuthenticatorDelegate;
import java.util.Arrays;

public class PasswordAuthenticator implements ListenerPasswordAuthenticatorDelegate {
    public boolean authenticate(String username, char[] password) {
        return (username.equals("testkit")) && Arrays.equals(password, "pass".toCharArray());
    }
}