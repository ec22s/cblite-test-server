package com.couchbase.mobiletestkit.javacommon.RequestHandler;

import com.couchbase.lite.Collection;
import com.couchbase.lite.CouchbaseLiteException;
import com.couchbase.lite.Database;
import com.couchbase.lite.Scope;
import com.couchbase.mobiletestkit.javacommon.Args;

public class ScopeRequestHandler {
    /* ___________*/
    /* - Scope - */
    /* __________*/
    public String scopeName(Args args) throws CouchbaseLiteException {
        Scope scope = args.get("scope");
        return scope.getName();
    }

    public Scope defaultScope(Args args) throws CouchbaseLiteException {
        Database db = args.get("database");
        return db.getDefaultScope();
    }

    public Collection collection(Args args) throws CouchbaseLiteException {
        Scope scope = args.get("scope");
        String collectionName = args.get("collectionName");
        return scope.getCollection(collectionName);
    }
}