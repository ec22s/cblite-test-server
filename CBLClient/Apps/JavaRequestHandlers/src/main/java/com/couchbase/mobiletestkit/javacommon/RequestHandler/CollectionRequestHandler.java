package com.couchbase.mobiletestkit.javacommon.RequestHandler;

import java.util.ArrayList;
import java.util.Date;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Set;

import com.couchbase.lite.Blob;
import com.couchbase.lite.CouchbaseLiteException;
import com.couchbase.lite.DataSource;
import com.couchbase.lite.Database;
import com.couchbase.lite.Expression;
import com.couchbase.lite.Index;
import com.couchbase.lite.IndexConfiguration;
import com.couchbase.lite.Limit;
import com.couchbase.lite.Meta;
import com.couchbase.lite.Query;
import com.couchbase.lite.QueryBuilder;
import com.couchbase.lite.Result;
import com.couchbase.lite.ResultSet;
import com.couchbase.lite.Scope;
import com.couchbase.lite.SelectResult;
import com.couchbase.lite.QueryIndex;
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

    public List<Collection> collectionInstances(Args args) throws CouchbaseLiteException {
        String scopeName = (args.get("scopeName") != null) ? args.get("scopeName") : "_default";
        Database db = args.get("database");
        Set<Collection> setCollections = db.getCollections(scopeName);
        List<Collection> returnCollections = new ArrayList<Collection>();
        returnCollections.addAll(setCollections);
        return returnCollections;
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
        Database db = args.get("database");
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

    public List<String> getDocIds(Args args) throws CouchbaseLiteException {
        Collection collection = args.get("collection");
        int limit = args.get("limit");
        int offset = args.get("offset");
        Query query = QueryBuilder
                .select(SelectResult.expression(Meta.id))
                .from(DataSource.collection(collection))
                .limit(Expression.intValue(limit), Expression.intValue(offset));
        List<String> result = new ArrayList<>();
        ResultSet results = query.execute();
        for (Result row : results) {

            result.add(row.getString("id"));
        }
        return result;
    }

    public Map<String, Map<String, Object>> getDocuments(Args args) throws CouchbaseLiteException {
        Collection collection = args.get("collection");
        List<String> ids = args.get("ids");
        Map<String, Map<String, Object>> documents = new HashMap<>();
        for (String id : ids) {
            Document document = collection.getDocument(id);
            if (document != null) {
                Map<String, Object> doc = document.toMap();
                // looping through the document, replace the Blob with its properties
                for (Map.Entry<String, Object> entry : doc.entrySet()) {
                    if (entry.getValue() != null && entry.getValue() instanceof Map<?, ?>) {
                        if (((Map) entry.getValue()).size() == 0) {
                            continue;
                        }
                        boolean isBlob = false;
                        Map<?, ?> value = (Map<?, ?>) entry.getValue();
                        Map<String, Object> newVal = new HashMap<>();
                        for (Map.Entry<?, ?> item : value.entrySet()) {
                            if (item.getValue() != null && item.getValue() instanceof Blob) {
                                isBlob = true;
                                Blob b = (Blob) item.getValue();
                                newVal.put(item.getKey().toString(), b.getProperties());
                            }
                        }
                        if (isBlob) { 
                            doc.put(entry.getKey(), newVal); 
                        }
                    }
                }
                documents.put(id, doc);
            }
        }
        return documents;
    }

    public void updateDocument(Args args) throws CouchbaseLiteException {
        Collection collection = args.get("collection");
        String id = args.get("id");
        Map<String, Object> data = args.get("data");
        MutableDocument updatedDoc = collection.getDocument(id).toMutable();
        Map<String, Object> new_data = this.setDataBlob(data);
        updatedDoc.setData(new_data);
        collection.save(updatedDoc);
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

    public QueryIndex getIndex(Args args) throws CouchbaseLiteException {
        Collection collection = args.get("collection");
        String name = args.get("indexName");
        return collection.getIndex(name);
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
            if (attItemValue.get("data") != null) {
                String contentType = attItemKey.endsWith(".png") ? "image/jpeg": "text/plain";
                Blob blob = new Blob(contentType,
                        RequestHandlerDispatcher.context.decodeBase64(attItemValue.get("data")));
                data.put(attItemKey, blob);

            }
            else if (attItemValue.containsKey("digest")) {
                existingBlobItems.put(attItemKey, attItemValue);
            }
        }
        data.remove("_attachments");
        // deal with partial blob situation,
        // remove all elements then add back blob type only items to _attachments key
        if (existingBlobItems.size() > 0 && existingBlobItems.size() < attachment_items.size()) {
            data.remove("_attachments");
            data.put("_attachments", existingBlobItems);
        }

        return data;
    }
}