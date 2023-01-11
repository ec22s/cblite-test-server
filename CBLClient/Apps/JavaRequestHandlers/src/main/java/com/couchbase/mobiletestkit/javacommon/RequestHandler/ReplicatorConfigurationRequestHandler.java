package com.couchbase.mobiletestkit.javacommon.RequestHandler;



import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.net.URI;
import java.net.URISyntaxException;
import java.util.ArrayList;
import java.util.Dictionary;
import java.util.EnumSet;
import java.util.List;
import java.util.Map;

import com.couchbase.lite.Collection;
import com.couchbase.lite.CollectionConfiguration;
import com.couchbase.lite.Endpoint;
import com.couchbase.lite.Replicator;
import com.couchbase.mobiletestkit.javacommon.Args;
import com.couchbase.mobiletestkit.javacommon.RequestHandlerDispatcher;
import com.couchbase.mobiletestkit.javacommon.util.Log;
import com.couchbase.lite.Authenticator;
import com.couchbase.lite.Conflict;
import com.couchbase.lite.ConflictResolver;
import com.couchbase.lite.Database;
import com.couchbase.lite.DatabaseEndpoint;
import com.couchbase.lite.Document;
import com.couchbase.lite.DocumentFlag;
import com.couchbase.lite.MutableDocument;
import com.couchbase.lite.ReplicationFilter;
import com.couchbase.lite.ReplicatorType;
import com.couchbase.lite.ReplicatorConfiguration;
import com.couchbase.lite.URLEndpoint;

import static java.lang.Thread.sleep;


public class ReplicatorConfigurationRequestHandler {
    private static final String TAG = "REPLCONFIGHANDLER";
    public void addCollection(Args args) throws Exception {
        ReplicatorConfiguration config = args.get("replicatorConfiguration");
        Collection collection = args.get("collections");
        CollectionConfiguration configuration = args.get("configurations");
        if (configuration != null && collection != null) {
            config.addCollection(collection, configuration);
        }
        else if (collection != null) {
            config.addCollection(collection, null);
        }
        return;
    }

    public CollectionConfiguration collection(Args args) throws Exception {
        Boolean pull_filter = args.get("pull_filter");
        Boolean push_filter = args.get("push_filter");
        String conflict_resolver = args.get("conflictResolver");
        String filter_callback_func = args.get("filter_callback_func");
        List<String> channels = args.get("channels");
        List<String> documentIds = args.get("documentIDs");
        CollectionConfiguration config = new CollectionConfiguration();
        if (channels != null) {
            config.setChannels(channels);
        }
        if (documentIds != null) {
            config.setDocumentIDs(documentIds);
        }
        if (push_filter) {
            switch (filter_callback_func) {
                case "boolean":
                    config.setPushFilter(new ReplicatorBooleanFilterCallback());
                    break;
                case "deleted":
                    config.setPushFilter(new ReplicatorDeletedFilterCallback());
                    break;
                case "access_revoked":
                    config.setPushFilter(new ReplicatorAccessRevokedFilterCallback());
                    break;
                default:
                    config.setPushFilter(new DefaultReplicatorFilterCallback());
                    break;
            }
        }
        if (pull_filter) {
            switch (filter_callback_func) {
                case "boolean":
                    config.setPullFilter(new ReplicatorBooleanFilterCallback());
                    break;
                case "deleted":
                    config.setPullFilter(new ReplicatorDeletedFilterCallback());
                    break;
                case "access_revoked":
                    config.setPullFilter(new ReplicatorAccessRevokedFilterCallback());
                    break;
                default:
                    config.setPullFilter(new DefaultReplicatorFilterCallback());
                    break;
            }
        }
        if (conflict_resolver != null) {
            switch (conflict_resolver) {
                case "local_wins":
                    config.setConflictResolver(new LocalWinsCustomConflictResolver());
                    break;
                case "remote_wins":
                    config.setConflictResolver(new RemoteWinsCustomConflictResolver());
                    break;
                case "null":
                    config.setConflictResolver(new NullCustomConflictResolver());
                    break;
                case "merge":
                    config.setConflictResolver(new MergeCustomConflictResolver());
                    break;
                case "incorrect_doc_id":
                    config.setConflictResolver(new IncorrectDocIdConflictResolver());
                    break;
                case "delayed_local_win":
                    config.setConflictResolver(new DelayedLocalWinConflictResolver());
                    break;
                case "delete_not_win":
                    config.setConflictResolver(new DeleteDocConflictResolver());
                    break;
                case "exception_thrown":
                    config.setConflictResolver(new ExceptionThrownConflictResolver());
                    break;
                default:
                    config.setConflictResolver(ConflictResolver.DEFAULT);
                    break;
            }
        }
        else {
            config.setConflictResolver(ConflictResolver.DEFAULT);
        }
        return config;
    }

    public Replicator configureCollection(Args args) throws Exception {
        Boolean continuous = args.get("continuous");
        URI target_url = null;
        String max_retries = args.get("max_retries");
        String max_timeout = args.get("max_timeout");
        String heartbeat = args.get("heartbeat");
        Authenticator authenticator = args.get("authenticator");
        String replication_type = args.get("replication_type");
        Map<String, String> headers = args.get("headers");
        target_url = new URI((String) args.get("target_url"));
        Database target_db = null;
        if (args.get("target_db") != null) {
            target_db = args.get("target_db");
        }
        String auto_purge = args.get("auto_purge");
        String pinnedservercert = args.get("pinnedservercert");
        List<com.couchbase.lite.Collection> collections = args.get("collections");
        List<CollectionConfiguration> configuration = args.get("configuration");
        if (replication_type == null) {
            replication_type = "push_pull";
        }
        replication_type = replication_type.toLowerCase();
        ReplicatorType replType;
        if (replication_type.equals("push")) {
            replType = ReplicatorType.PUSH;
        }
        else if (replication_type.equals("pull")) {
            replType = ReplicatorType.PULL;
        }
        else {
            replType = ReplicatorType.PUSH_AND_PULL;
        }
        ReplicatorConfiguration config = null;
        Endpoint target = new URLEndpoint((target_url));
        if (target_db != null) {
            target = new DatabaseEndpoint(target_db);
            config = new ReplicatorConfiguration(target);
        }
        if (target == null) {
            throw new Exception("\"Target Url or Target Database is required\"");
        }
        config = new ReplicatorConfiguration(target);
        config.setType(replType);
        if( continuous != null) {
            config.setContinuous(continuous);
        }
        else {
            config.setContinuous((false));
        }
        if (headers != null) {
            config.setHeaders(headers);
        }
        if(authenticator != null) {
            config.setAuthenticator(authenticator);
        }
        if(heartbeat != null) {
            Integer heartBeat = Integer.parseInt(heartbeat);
            config.setHeartbeat(heartBeat);
        }
        if (max_retries != null) {
            config.setMaxAttempts(Integer.parseInt(max_retries));
        }
        if (auto_purge != null) {
            boolean autoPurge = auto_purge.toLowerCase() == "enabled";
            config.setAutoPurgeEnabled(autoPurge);
        }
        if (max_timeout != null) {
            Integer maxTimeout = Integer.parseInt(max_timeout);
            config.setMaxAttemptWaitTime(maxTimeout);
        }
        Integer size_collection = collections.size();
        Integer size_configuration = configuration.size();
        if (collections != null) {
            if (configuration.size()>1 && configuration.size() == collections.size()) {
                for (int i = 0; i < collections.size(); i++) {
                    if (i< configuration.size()) {
                        config.addCollection(collections.get(i), configuration.get(i));
                    }
                    else {
                        config.addCollection(collections.get(i), null);
                    }
                }
            }
            else if (configuration.size()==1 && collections.size()>1) {
                config.addCollections(collections, configuration.get(0));
            }
            else if (configuration.size() == 1 && collections.size() ==1) {
                config.addCollections(collections, configuration.get(0));
            }
            else if(configuration == null) {
                config.addCollections(collections, null);
            }
            else {
                throw new Exception("\"Mismatch in number of collections and configurations\"");
            }
        }

        return new Replicator(config);
    }
    public ReplicatorConfiguration builderCreate(Args args) throws URISyntaxException {
        Database sourceDb = args.get("sourceDb");
        Database targetDb = args.get("targetDb");
        URI targetURI = null;
        if (args.get("targetURI") != null) {
            targetURI = new URI((String) args.get("targetURI"));
        }
        if (targetDb != null) {
            DatabaseEndpoint target = new DatabaseEndpoint(targetDb);
            return new ReplicatorConfiguration(sourceDb, target);
        }
        else if (targetURI != null) {
            URLEndpoint target = new URLEndpoint(targetURI);
            return new ReplicatorConfiguration(sourceDb, target);
        }
        else {
            throw new IllegalArgumentException("Incorrect configuration parameter provided");
        }
    }

    public ReplicatorConfiguration configure(Args args) throws Exception {
        Database sourceDb = args.get("source_db");
        URI targetURL = null;
        if (args.get("target_url") != null) {
            targetURL = new URI((String) args.get("target_url"));
        }
        Database targetDb = args.get("target_db");
        String replicatorType = args.get("replication_type");
        Boolean continuous = args.get("continuous");
        List<String> channels = args.get("channels");
        List<String> documentIds = args.get("documentIDs");
        String pinnedservercert = args.get("pinnedservercert");
        Authenticator authenticator = args.get("authenticator");
        Boolean push_filter = args.get("push_filter");
        Boolean pull_filter = args.get("pull_filter");
        String filter_callback_func = args.get("filter_callback_func");
        String conflict_resolver = args.get("conflict_resolver");
        Map<String, String> headers = args.get("headers");
        String heartbeat = args.get("heartbeat");
        String maxRetries = args.get("max_retries");
        String maxRetryWaitTime = args.get("max_timeout");
        String auto_purge = args.get("auto_purge");

        if (replicatorType == null) {
            replicatorType = "push_pull";
        }
        replicatorType = replicatorType.toLowerCase();
        ReplicatorType replType;
        if (replicatorType.equals("push")) {
            replType = ReplicatorType.PUSH;
        }
        else if (replicatorType.equals("pull")) {
            replType = ReplicatorType.PULL;
        }
        else {
            replType = ReplicatorType.PUSH_AND_PULL;
        }
        ReplicatorConfiguration config;
        if (sourceDb != null && targetURL != null) {
            URLEndpoint target = new URLEndpoint(targetURL);
            config = new ReplicatorConfiguration(sourceDb, target);
        }
        else if (sourceDb != null && targetDb != null) {
            DatabaseEndpoint target = new DatabaseEndpoint(targetDb);
            config = new ReplicatorConfiguration(sourceDb, target);
        }
        else {
            throw new Exception("\"No source db provided or target url provided\"");
        }
        if (continuous != null) {
            config.setContinuous(continuous);
        }
        else {
            config.setContinuous(false);
        }
        if (headers != null) {
            config.setHeaders(headers);
        }
        if (authenticator != null) {
            config.setAuthenticator(authenticator);
        }
        config.setType(replType);
        /*if (conflictResolver != null) {
            config.setConflictResolver(conflictResolver);
        }*/
        if (channels != null) {
            config.setChannels(channels);
        }
        if (documentIds != null) {
            config.setDocumentIDs(documentIds);
        }
        if (heartbeat != null && !heartbeat.trim().isEmpty()){
            config.setHeartbeat(Integer.parseInt(heartbeat));
        }
        if (maxRetries != null && !maxRetries.trim().isEmpty()){
            config.setMaxAttempts(Integer.parseInt(maxRetries));
        }
        if (maxRetryWaitTime != null && !maxRetryWaitTime.trim().isEmpty()){
            config.setMaxAttemptWaitTime(Integer.parseInt(maxRetryWaitTime));
        }
        if (auto_purge != null){
            if (auto_purge.equalsIgnoreCase("enabled")){
                Log.i(TAG, "auto purge is enabled explicitly");
                config.setAutoPurgeEnabled(true);
            }
            else if (auto_purge.equalsIgnoreCase("disabled")){
                Log.i(TAG, "auto purge is disabled");
                config.setAutoPurgeEnabled(false);
            }
        }
        else {
            Log.i(TAG, "auto purge not specified, use the default setting.");
        }

        Log.d(TAG, "Args: " + args);
        if (pinnedservercert != null) {
            byte[] ServerCert = this.getPinnedCertFile();
            // Set pinned certificate.
            config.setPinnedServerCertificate(ServerCert);
        }
        if (push_filter) {
            switch (filter_callback_func) {
                case "boolean":
                    config.setPushFilter(new ReplicatorBooleanFilterCallback());
                    break;
                case "deleted":
                    config.setPushFilter(new ReplicatorDeletedFilterCallback());
                    break;
                case "access_revoked":
                    config.setPushFilter(new ReplicatorAccessRevokedFilterCallback());
                    break;
                default:
                    config.setPushFilter(new DefaultReplicatorFilterCallback());
                    break;
            }
        }
        if (pull_filter) {
            switch (filter_callback_func) {
                case "boolean":
                    config.setPullFilter(new ReplicatorBooleanFilterCallback());
                    break;
                case "deleted":
                    config.setPullFilter(new ReplicatorDeletedFilterCallback());
                    break;
                case "access_revoked":
                    config.setPullFilter(new ReplicatorAccessRevokedFilterCallback());
                    break;
                default:
                    config.setPullFilter(new DefaultReplicatorFilterCallback());
                    break;
            }
        }
        switch (conflict_resolver) {
            case "local_wins":
                config.setConflictResolver(new LocalWinsCustomConflictResolver());
                break;
            case "remote_wins":
                config.setConflictResolver(new RemoteWinsCustomConflictResolver());
                break;
            case "null":
                config.setConflictResolver(new NullCustomConflictResolver());
                break;
            case "merge":
                config.setConflictResolver(new MergeCustomConflictResolver());
                break;
            case "incorrect_doc_id":
                config.setConflictResolver(new IncorrectDocIdConflictResolver());
                break;
            case "delayed_local_win":
                config.setConflictResolver(new DelayedLocalWinConflictResolver());
                break;
            case "delete_not_win":
                config.setConflictResolver(new DeleteDocConflictResolver());
                break;
            case "exception_thrown":
                config.setConflictResolver(new ExceptionThrownConflictResolver());
                break;
            default:
                config.setConflictResolver(ConflictResolver.DEFAULT);
                break;
        }
        return config;
    }

    public ReplicatorConfiguration create(Args args) {
        return args.get("configuration");
    }

    public Authenticator getAuthenticator(Args args) {
        ReplicatorConfiguration replicatorConfiguration = args.get("configuration");
        return replicatorConfiguration.getAuthenticator();
    }

    public List<String> getChannels(Args args) {
        ReplicatorConfiguration replicatorConfiguration = args.get("configuration");
        return replicatorConfiguration.getChannels();
    }

    /*public ConflictResolver getConflictResolver(Args args){
        ReplicatorConfiguration replicatorConfiguration = args.get("configuration");
        return replicatorConfiguration.getConflictResolver();
    }*/

    public Database getDatabase(Args args) {
        ReplicatorConfiguration replicatorConfiguration = args.get("configuration");
        return replicatorConfiguration.getDatabase();
    }

    public List<String> getDocumentIDs(Args args) {
        ReplicatorConfiguration replicatorConfiguration = args.get("configuration");
        return replicatorConfiguration.getDocumentIDs();
    }

    public byte[] getPinnedServerCertificate(Args args) {
        ReplicatorConfiguration replicatorConfiguration = args.get("configuration");
        return replicatorConfiguration.getPinnedServerCertificate();
    }

    public String getReplicatorType(Args args) {
        ReplicatorConfiguration replicatorConfiguration = args.get("configuration");
        return replicatorConfiguration.getType().toString();
    }

    public String getTarget(Args args) {
        ReplicatorConfiguration replicatorConfiguration = args.get("configuration");
        return replicatorConfiguration.getTarget().toString();
    }

    public Boolean isContinuous(Args args) {
        ReplicatorConfiguration replicatorConfiguration = args.get("configuration");
        return replicatorConfiguration.isContinuous();
    }

    public void setAuthenticator(Args args) {
        ReplicatorConfiguration replicatorConfiguration = args.get("configuration");
        Authenticator authenticator = args.get("authenticator");
        replicatorConfiguration.setAuthenticator(authenticator);
    }

    public void setChannels(Args args) {
        ReplicatorConfiguration replicatorConfiguration = args.get("configuration");
        List<String> channels = args.get("channels");
        replicatorConfiguration.setChannels(channels);
    }

    /*public void setConflictResolver(Args args){
        ReplicatorConfiguration replicatorConfiguration = args.get("configuration");
        ConflictResolver conflictResolver = args.get("conflictResolver");
        replicatorConfiguration.setConflictResolver(conflictResolver);
    }*/

    public void setContinuous(Args args) {
        ReplicatorConfiguration replicatorConfiguration = args.get("configuration");
        Boolean continuous = args.get("continuous");
        replicatorConfiguration.setContinuous(continuous);
    }

    public void setDocumentIDs(Args args) {
        ReplicatorConfiguration replicatorConfiguration = args.get("configuration");
        List<String> documentIds = args.get("documentIds");
        replicatorConfiguration.setDocumentIDs(documentIds);
    }

    public void setPinnedServerCertificate(Args args) {
        ReplicatorConfiguration replicatorConfiguration = args.get("configuration");
        byte[] cert = args.get("cert");
        replicatorConfiguration.setPinnedServerCertificate(cert);
    }

    public void setReplicatorType(Args args) {
        ReplicatorConfiguration replicatorConfiguration = args.get("configuration");
        String type = args.get("replType");
        ReplicatorType replicatorType;
        switch (type) {
            case "PUSH":
                replicatorType = ReplicatorType.PUSH;
                break;
            case "PULL":
                replicatorType = ReplicatorType.PULL;
                break;
            default:
                replicatorType = ReplicatorType.PUSH_AND_PULL;
        }
        replicatorConfiguration.setType(replicatorType);
    }

    public void setAutoPurge(Args args) {
        ReplicatorConfiguration replicatorConfiguration = args.get("configuration");
        Boolean auto_purge = args.get("auto_purge");
        replicatorConfiguration.setAutoPurgeEnabled(auto_purge);
    }

    private byte[] getPinnedCertFile() {
        InputStream is = null;
        try {
            is = RequestHandlerDispatcher.context.getAsset("sg_cert.cer");
            return toByteArray(is);
        }
        finally {
            if (is != null) { try { is.close(); } catch (IOException e) { } }
        }
    }

    public static byte[] toByteArray(InputStream is) {
        ByteArrayOutputStream bos = new ByteArrayOutputStream();
        byte[] b = new byte[1024];

        try {
            int bytesRead = is.read(b);
            while (bytesRead != -1) {
                bos.write(b, 0, bytesRead);
                bytesRead = is.read(b);
            }
        }
        catch (IOException io) {
            Log.w(TAG, "Got exception " + io.getMessage() + ", Ignoring...");
        }

        return bos.toByteArray();
    }
}

class ReplicatorBooleanFilterCallback implements ReplicationFilter {
    @Override
    public boolean filtered(Document document, EnumSet<DocumentFlag> flags) {
        String key = "new_field_1";
        if (document.contains(key)) {
            return document.getBoolean(key);
        }
        return true;
    }

}

class DefaultReplicatorFilterCallback implements ReplicationFilter {
    @Override
    public boolean filtered(Document document, EnumSet<DocumentFlag> flags) {
        return true;
    }

}

class ReplicatorDeletedFilterCallback implements ReplicationFilter {
    @Override
    public boolean filtered(Document document, EnumSet<DocumentFlag> flags) {
        return !(flags.contains(DocumentFlag.DELETED));
    }
}

class ReplicatorAccessRevokedFilterCallback implements ReplicationFilter {
    @Override
    public boolean filtered(Document document, EnumSet<DocumentFlag> flags) {
        return !(flags.contains(DocumentFlag.ACCESS_REMOVED));
    }
}

class LocalWinsCustomConflictResolver implements ConflictResolver {
    private static final String TAG = "CCRREPLCONFIGHANDLER";
    @Override
    public Document resolve(Conflict conflict) {
        Document localDoc = conflict.getLocalDocument();
        Document remoteDoc = conflict.getRemoteDocument();
        if (localDoc == null || remoteDoc == null) {
            throw new IllegalStateException("Either local doc or remote is/are null");
        }
        String docId = conflict.getDocumentId();
        Utility util_obj = new Utility();
        util_obj.checkMismatchDocId(localDoc, remoteDoc, docId);
        return localDoc;
    }
}

class RemoteWinsCustomConflictResolver implements ConflictResolver {
    private static final String TAG = "CCRREPLCONFIGHANDLER";
    @Override
    public Document resolve(Conflict conflict) {
        Document localDoc = conflict.getLocalDocument();
        Document remoteDoc = conflict.getRemoteDocument();
        if (localDoc == null || remoteDoc == null) {
            throw new IllegalStateException("Either local doc or remote is/are null");
        }
        String docId = conflict.getDocumentId();
        Utility util_obj = new Utility();
        util_obj.checkMismatchDocId(localDoc, remoteDoc, docId);
        return remoteDoc;
    }
}

class NullCustomConflictResolver implements ConflictResolver {
    private static final String TAG = "CCRREPLCONFIGHANDLER";
    @Override
    public Document resolve(Conflict conflict) {
        Document localDoc = conflict.getLocalDocument();
        Document remoteDoc = conflict.getRemoteDocument();
        if (localDoc == null || remoteDoc == null) {
            throw new IllegalStateException("Either local doc or remote is/are null");
        }
        String docId = conflict.getDocumentId();
        Utility util_obj = new Utility();
        util_obj.checkMismatchDocId(localDoc, remoteDoc, docId);
        return null;
    }
}

class MergeCustomConflictResolver implements ConflictResolver {
    private static final String TAG = "CCRREPLCONFIGHANDLER";
    @Override
    public Document resolve(Conflict conflict) {
        /**
         * Migrate the conflicted doc.
         * Algorithm creates a new doc with copying local doc and then adding any additional key
         * from remote doc. Conflicting keys will have value from local doc.
         */
        Document localDoc = conflict.getLocalDocument();
        Document remoteDoc = conflict.getRemoteDocument();
        if (localDoc == null || remoteDoc == null) {
            throw new IllegalStateException("Either local doc or remote is/are null");
        }
        String docId = conflict.getDocumentId();
        Utility util_obj = new Utility();
        util_obj.checkMismatchDocId(localDoc, remoteDoc, docId);
        MutableDocument newDoc = localDoc.toMutable();
        Map<String, Object> remoteDocMap = remoteDoc.toMap();
        for (Map.Entry<String, Object> entry : remoteDocMap.entrySet()) {
            String key = entry.getKey();
            Object value = entry.getValue();
            if (!newDoc.contains(key)) {
                newDoc.setValue(key, value);
            }
        }
        return newDoc;
    }
}

class IncorrectDocIdConflictResolver implements ConflictResolver {
    private static final String TAG = "CCRREPLCONFIGHANDLER";
    @Override
    public Document resolve(Conflict conflict) {
        Document localDoc = conflict.getLocalDocument();
        Document remoteDoc = conflict.getRemoteDocument();
        if (localDoc == null || remoteDoc == null) {
            throw new IllegalStateException("Either local doc or remote is/are null");
        }
        String docId = conflict.getDocumentId();
        Utility util_obj = new Utility();
        util_obj.checkMismatchDocId(localDoc, remoteDoc, docId);
        String newId = "changed" + docId;
        MutableDocument newDoc = new MutableDocument(newId, localDoc.toMap());
        newDoc.setValue("new_value", "couchbase");
        return newDoc;
    }
}

class DelayedLocalWinConflictResolver implements ConflictResolver {
    private static final String TAG = "CCRREPLCONFIGHANDLER";
    @Override
    public Document resolve(Conflict conflict) {
        Document localDoc = conflict.getLocalDocument();
        Document remoteDoc = conflict.getRemoteDocument();
        if (localDoc == null || remoteDoc == null) {
            throw new IllegalStateException("Either local doc or remote is/are null");
        }
        String docId = conflict.getDocumentId();
        Utility util_obj = new Utility();
        util_obj.checkMismatchDocId(localDoc, remoteDoc, docId);
        try {
            sleep(1000 * 10);
        } catch (InterruptedException e) {
            e.printStackTrace();
        }
        return localDoc;
    }
}

class DeleteDocConflictResolver implements ConflictResolver {
    private static final String TAG = "CCRREPLCONFIGHANDLER";
    @Override
    public Document resolve(Conflict conflict) {
        Document localDoc = conflict.getLocalDocument();
        Document remoteDoc = conflict.getRemoteDocument();
        if (localDoc == null || remoteDoc == null) {
            throw new IllegalStateException("Either local doc or remote is/are null");
        }
        String docId = conflict.getDocumentId();
        Utility util_obj = new Utility();
        util_obj.checkMismatchDocId(localDoc, remoteDoc, docId);
        if (remoteDoc == null) {
            return localDoc;
        } else {
            return null;
        }
    }
}

class ExceptionThrownConflictResolver implements ConflictResolver {
    private static final String TAG = "CCRREPLCONFIGHANDLER";
    @Override
    public Document resolve(Conflict conflict) {
        Document localDoc = conflict.getLocalDocument();
        Document remoteDoc = conflict.getRemoteDocument();
        if (localDoc == null || remoteDoc == null) {
            throw new IllegalStateException("Either local doc or remote is/are null");
        }
        String docId = conflict.getDocumentId();
        Utility util_obj = new Utility();
        util_obj.checkMismatchDocId(localDoc, remoteDoc, docId);
        throw new IllegalStateException("Throwing an exception");
    }
}

class Utility {
    public void checkMismatchDocId(Document localDoc, Document remoteDoc, String docId) {
        String remoteDocId = remoteDoc.getId();
        String localDocId = localDoc.getId();
        if (remoteDocId != docId) {
            throw new IllegalStateException("DocId mismatch");
        }
        if (docId != localDocId) {
            throw new IllegalStateException("DocId mismatch");
        }
    }
}
