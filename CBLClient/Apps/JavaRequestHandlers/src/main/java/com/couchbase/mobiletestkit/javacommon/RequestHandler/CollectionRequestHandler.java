package com.couchbase.mobiletestkit.javacommon.RequestHandler;

import java.util.ArrayList;
import java.util.Date;
import java.util.HashMap;
import java.util.Map;
import java.util.Set;

import com.couchbase.lite.Blob;
import com.couchbase.lite.CouchbaseLiteException;
import com.couchbase.lite.Database;
import com.couchbase.lite.IndexConfiguration;
import com.couchbase.lite.Scope;
import com.couchbase.lite.ValueIndexConfiguration;
import com.couchbase.mobiletestkit.javacommon.Args;
import com.couchbase.lite.Collection;

import com.couchbase.lite.Document;
import com.couchbase.lite.MutableDocument;
import com.couchbase.mobiletestkit.javacommon.RequestHandlerDispatcher;
import com.couchbase.mobiletestkit.javacommon.util.Log;

public class CollectionRequestHandler {
    private static final String TAG = "DATABASE";
    /* _______________*/
    /* - Collection - */
    /* _______________*/
    public String getCollectionName(Args args) throws CouchbaseLiteException {
        Collection object = args.get("collection");
        return object.getName();
    }

    public Collection createCollection(Args args) throws CouchbaseLiteException {
        Database db = args.get("database");
        String collectionName = args.get("collectionName");
        String scopeName = args.get("scopeName");
        if (scopeName == null) {
            return db.createCollection(collectionName);
        }
        else {
            return db.createCollection(collectionName, scopeName);
        }
    }

    public void deleteCollection(Args args) throws CouchbaseLiteException {
        Database db = args.get("database");
        String collectionName = args.get("collectionName");
        String scopeName = args.get("scopeName");
        if (scopeName == null) {
            db.deleteCollection(collectionName);
            return;
        }
        else {
            db.deleteCollection(collectionName, scopeName);
            return;
        }
    }

    public Collection defaultCollection(Args args) throws CouchbaseLiteException {
        Database db =args.get("database");
        return db.getDefaultCollection();

    }

    public static Set<Collection> collectionsInScope(Database db, String scopeName) throws CouchbaseLiteException {
        if (scopeName == null) {
            return db.getCollections();
        }
        else {
            return db.getCollections(scopeName);
        }
    }

    public ArrayList<String> collectionNames(Args args) throws CouchbaseLiteException {
        Database db = args.get("database");
        String scopeName = args.get("scopeName");
        ArrayList<String> collectionsNames = new ArrayList<String>();
        Set<Collection> collectionObjects = collectionsInScope(db, scopeName);
        for (Collection collectionObject: collectionObjects) {
            collectionsNames.add(collectionObject.getName());
        }
        return collectionsNames;
    }

    public long documentCount(Args args) throws CouchbaseLiteException {
        Collection collectionObject = args.get("collection");
        return collectionObject.getCount();
    }

    public Collection collection(Args args) throws CouchbaseLiteException {
        Database db = args.get("database");
        String collectionName = args.get("collectionName");
        String scopeName = args.get("scopeName");
        if (scopeName == null) {
            return db.getCollection(collectionName);
        }
        else {
            return db.getCollection(collectionName, scopeName);
        }
    }

    public void saveDocument(Args args) throws CouchbaseLiteException {
        Collection collection = args.get("collection");
        MutableDocument document = args.get("document");
        collection.save(document);
        return;
    }

    public Scope collectionScope(Args args) throws CouchbaseLiteException {
        Collection collection = args.get("collection");
        return collection.getScope();
    }

    public Document getDocument(Args args) throws CouchbaseLiteException {
        Collection collection = args.get("collection");
        String docId = args.get("docId");
        return collection.getDocument(docId);
    }

    public void deleteDocument(Args args) throws CouchbaseLiteException {
        Collection collection = args.get("collection");
        Document doc = args.get("document");
        collection.delete(doc);
        return;
    }

    public void purgeDocument(Args args) throws CouchbaseLiteException {
        Collection collection = args.get("collection");
        Document doc = args.get("document");
        collection.purge(doc);
        return;
    }

    public void purgeDocumentID(Args args) throws CouchbaseLiteException {
        Collection collection = args.get("collection");
        String docID = args.get("docId");
        collection.purge(docID);
        return;
    }

    public Date getDocumentExpiration(Args args) throws CouchbaseLiteException {
        Collection collection = args.get("collection");
        String docId = args.get("docId");
        return collection.getDocumentExpiration(docId);
    }

    public void setDocumentExpiration(Args args) throws CouchbaseLiteException {
        Collection collection = args.get("collection");
        String docId = args.get("docId");
        Date expiration = args.get("expiration");
        collection.setDocumentExpiration(docId, expiration);
        return;
    }

    public void createValueIndex(Args args) throws CouchbaseLiteException {
        Collection collection = args.get("collection");
        String name = args.get("name");
        String expression = args.get("expression");
        IndexConfiguration expressionConfig = new ValueIndexConfiguration(expression);
        collection.createIndex(name, expressionConfig);
        return;
    }

    public void deleteIndex(Args args) throws CouchbaseLiteException {
        Collection collection = args.get("collection");
        String name = args.get("name");
        collection.deleteIndex(name);
        return;
    }

    public Set<String> getIndexNames(Args args) throws CouchbaseLiteException {
        Collection collection = args.get("collection");
        return collection.getIndexes();
    }

    public void saveDocuments(Args args) throws CouchbaseLiteException {
       final Collection collection = args.get("collection");
       final Database db = args.get("database");
       final Map<String, Map<String, Object>> documents = args.get("documents");
        db.inBatch(() -> {
            for (Map.Entry<String, Map<String, Object>> entry : documents.entrySet()) {
                String id = entry.getKey();
                Map<String, Object> data = entry.getValue();
                Map<String, Object> new_data = this.setDataBlob(data);
                MutableDocument document = new MutableDocument(id, new_data);
                try {
                    collection.save(document);
                }
                catch (CouchbaseLiteException e) {
                    Log.e(TAG, "DB Save failed", e);
                }
            }
        });
    }

    private Map<String, Object> setDataBlob(Map<String, Object> data) {
        if (!data.containsKey("_attachments")) {
            return data;
        }

        Map<String, Object> attachment_items = (Map<String, Object>) data.get("_attachments");
        Map<String, Object> existingBlobItems = new HashMap<>();

        for (Map.Entry<String, Object> attItem : attachment_items.entrySet()) {
            String attItemKey = attItem.getKey();
            HashMap<String, String> attItemValue = (HashMap<String, String>) attItem.getValue();
            if (attItemValue.get("data") != null){
                String contentType = attItemKey.endsWith(".png") ? "image/jpeg": "text/plain";
                Blob blob = new Blob(contentType,
                        RequestHandlerDispatcher.context.decodeBase64(attItemValue.get("data")));
                data.put(attItemKey, blob);

            }
            else if (attItemValue.containsKey("digest")){
                existingBlobItems.put(attItemKey, attItemValue);
            }
        }
        data.remove("_attachments");
        // deal with partial blob situation,
        // remove all elements then add back blob type only items to _attachments key
        if (existingBlobItems.size() > 0 && existingBlobItems.size() < attachment_items.size()){
            data.remove("_attachments");
            data.put("_attachments", existingBlobItems);
        }

        return data;
    }
}