package com.couchbase.mobiletestkit.javacommon.RequestHandler;

import com.couchbase.mobiletestkit.javacommon.Args;
import com.couchbase.lite.Collation;


public class CollatorRequestHandler {

    public Collation.ASCII ascii(Args args) {
        Boolean ignoreCase = args.get("ignoreCase");
        return Collation.ascii().setIgnoreCase(ignoreCase);
    }

    public Collation.Unicode unicode(Args args) {
        Boolean ignoreCase = args.get("ignoreCase");
        Boolean ignoreAccents = args.get("ignoreAccents");
        return Collation.unicode().setIgnoreCase(ignoreCase).setIgnoreAccents(ignoreAccents);
    }

}
