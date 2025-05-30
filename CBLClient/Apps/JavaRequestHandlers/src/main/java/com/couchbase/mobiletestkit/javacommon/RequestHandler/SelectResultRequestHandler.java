package com.couchbase.mobiletestkit.javacommon.RequestHandler;


import com.couchbase.mobiletestkit.javacommon.Args;
import com.couchbase.lite.Expression;
import com.couchbase.lite.SelectResult;


public class SelectResultRequestHandler {

    public SelectResult expressionCreate(Args args) {
        Expression expression = args.get("expression");
        return SelectResult.expression(expression);
    }

    public SelectResult all(Args args) {
        return SelectResult.all();
    }
}
