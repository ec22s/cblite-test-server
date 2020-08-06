package com.couchbase.mobiletestkit.javacommon.RequestHandler;

import com.couchbase.lite.DocumentChange;
import com.couchbase.lite.DocumentChangeListener;
import com.couchbase.lite.ListenerPasswordAuthenticator;
import com.couchbase.mobiletestkit.javacommon.Args;
import com.couchbase.lite.ListenerPasswordAuthenticator;

import java.util.List;

public class ListenerAuthenticatorRequestHandler {

    public ListenerPasswordAuthenticator create(Args args) {
        String username = args.get("username");
        String pass = args.get("password");
        char[] password = pass.toCharArray();
        PasswordAuthenticator passwordAuthenticator = new PasswordAuthenticator();
        ListenerPasswordAuthenticator listenerPasswordAuthenticator = ListenerPasswordAuthenticator.create(
                passwordAuthenticator);
        listenerPasswordAuthenticator.authenticate(username, password);
        System.out.println(listenerPasswordAuthenticator);
        System.out.println("PASSSSSSS");
        return listenerPasswordAuthenticator;
    }


}


