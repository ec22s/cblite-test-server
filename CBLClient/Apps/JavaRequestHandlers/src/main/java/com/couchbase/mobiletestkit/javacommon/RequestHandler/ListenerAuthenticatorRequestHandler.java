package com.couchbase.mobiletestkit.javacommon.RequestHandler;

import com.couchbase.lite.ListenerPasswordAuthenticator;
import com.couchbase.mobiletestkit.javacommon.Args;

public class ListenerAuthenticatorRequestHandler {

    public ListenerPasswordAuthenticator create(Args args) {
        String username = args.get("username");
        String pass = args.get("password");
        char[] password = pass.toCharArray();
        PasswordAuthenticator passwordAuthenticator = new PasswordAuthenticator();
        ListenerPasswordAuthenticator listenerPasswordAuthenticator = new ListenerPasswordAuthenticator(
                passwordAuthenticator);
//        listenerPasswordAuthenticator.authenticate(username, password);
        System.out.println("*******");
        return listenerPasswordAuthenticator;
    }

}
