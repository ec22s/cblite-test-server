package com.couchbase.mobiletestkit.javacommon.RequestHandler;

import com.couchbase.lite.DocumentChange;
import com.couchbase.lite.DocumentChangeListener;
import com.couchbase.lite.ListenerPasswordAuthenticator;
import com.couchbase.mobiletestkit.javacommon.Args;

import java.util.List;

public class ListenerAuthenticatorRequestHandler {

    public ListenerPasswordAuthenticator create(Args args) {
        String username = args.get("username");
        String password = args.get("password");
        return ListenerPasswordAuthenticator.authenticate(username, password);
    }


}


