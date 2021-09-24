package com.couchbase.mobiletestkit.javacommon.RequestHandler;
import com.couchbase.lite.ListenerPasswordAuthenticator;
import com.couchbase.mobiletestkit.javacommon.Args;
import com.couchbase.mobiletestkit.javacommon.util.PasswordAuthenticator;

import java.util.Arrays;


public class ListenerAuthenticatorRequestHandler {

    public ListenerPasswordAuthenticator create(Args args) {
        String username = args.get("username");
        String pass = args.get("password");
        char[] password = pass.toCharArray();
        return new ListenerPasswordAuthenticator(
                (validUser, validPassword) ->
                        username.equals(validUser) && Arrays.equals(password, validPassword));
    }

}