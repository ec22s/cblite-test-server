//
//  ReplicatorConfigurationRequestHandler.swift
//  CBLTestServer-iOS
//
//  Created by Raghu Sarangapani on 1/6/18.
//  Copyright © 2018 Raghu Sarangapani. All rights reserved.
//

import Foundation
import CouchbaseLiteSwift



public class ReplicatorConfigurationRequestHandler {
    public static let VOID: String? = nil
    fileprivate var _pushPullReplListener:NSObjectProtocol?
    
    public func handleRequest(method: String, args: Args) throws -> Any? {
        switch method {
        /////////////////////////////
        // ReplicatorConfiguration //
        /////////////////////////////
            
        // TODO: Change client to expect replicator config, not the builder.
        case "replicatorConfiguration_collection":
            let conflictResolver: ConflictResolverProtocol? = args.get(name: "conflictResolver")
            let pull_filter: Bool = args.get(name: "pull_filter")!
            let push_filter: Bool = args.get(name: "push_filter")!
            let filter_callback_func: String? = args.get(name: "filter_callback_func")
            let channels: [String]? = args.get(name: "channels")
            let documentIDs: [String]? = args.get(name: "documentIDs")
            var config = CollectionConfiguration()
            let filter1 = { (doc: Document, flags: DocumentFlags) in return false }
            config.conflictResolver = conflictResolver
            if (pull_filter != false) {
                if filter_callback_func == "boolean" {
                    config.pullFilter = _replicatorBooleanFilterCallback;
                } else if filter_callback_func == "deleted" {
                    config.pullFilter = _replicatorDeletedFilterCallback;
                } else if filter_callback_func == "access_revoked" {
                    config.pullFilter = _replicatorAccessRevokedCallback;
                } else {
                    config.pullFilter = _defaultReplicatorFilterCallback;
                }
            }
            
            if (push_filter != false) {
                if filter_callback_func == "boolean" {
                    config.pushFilter = _replicatorBooleanFilterCallback;
                } else if filter_callback_func == "deleted" {
                    config.pushFilter = _replicatorDeletedFilterCallback;
                } else if filter_callback_func == "access_revoked" {
                    config.pushFilter = _replicatorAccessRevokedCallback;
                } else {
                    config.pushFilter = _defaultReplicatorFilterCallback;
                }
            }
            config.channels = channels
            config.documentIDs = documentIDs
            return config
        
        case "replicatorConfiguration_addCollection":
            var replicatorConfiguration: ReplicatorConfiguration = args.get(name: "replicatorConfiguration")!
            let collection: Collection = args.get(name: "collections")!
            let collectionConfiguration: CollectionConfiguration? = args.get(name: "configuration")
            if(collectionConfiguration != nil) {
                replicatorConfiguration.addCollection((collection), config: collectionConfiguration)
            }
            else {
                replicatorConfiguration.addCollection(collection)
            }
        
        case "replicatorConfiguration_addCollections":
            var replicatorConfiguration: ReplicatorConfiguration = args.get(name: "replicatorConfiguration")!
            let collection: [Collection] = args.get(name: "collections")!
            let collectionConfiguration: CollectionConfiguration? = args.get(name: "configuration")
            replicatorConfiguration.addCollections((collection), config: collectionConfiguration)
        
        case "replicatorConfiguration_removeCollection":
            let collection: Collection = args.get(name: "collection")!
            var replicatorConfiguration: ReplicatorConfiguration = args.get(name: "replicatorConfiguration")!
            replicatorConfiguration.removeCollection(collection)
            
        case "replicatorConfiguration_collectionConfig":
            let collection: Collection = args.get(name: "collection")!
            let replicatorConfiguration: ReplicatorConfiguration = args.get(name: "replicator")!
            return replicatorConfiguration.collectionConfig(collection)
        
        case "replicatorConfiguration_collectionNames":
            let replicatorConfiguration: ReplicatorConfiguration = args.get(name: "replicator")!
            let collectionNames = replicatorConfiguration.collections
            var names = [String]()
            for collection in collectionNames {
                let collectionObject: Collection = collection
                names.append(collectionObject.name)
            }
            return names
            
        case "replicatorConfiguration_create":
            let sourceDb: Database = args.get(name: "sourceDb")!
            let targetURI: String? = args.get(name: "targetURI")!
            let targetDb: Database? = args.get(name: "targetDb")!
            var target: Endpoint?
            
            #if COUCHBASE_ENTERPRISE
            if (targetDb != nil) {
                target = DatabaseEndpoint(database: targetDb!)
            }
            #endif
            
            if (targetURI != nil) {
                target = URLEndpoint(url: URL(string: targetURI!)!)
            }
            
            if (target == nil) {
                throw RequestHandlerError.InvalidArgument("Target database or URL should be provided.")
            }
            
            return ReplicatorConfiguration(database: sourceDb, target: target!)
        
        case "replicatorConfiguration_configureCollection":
            Database.log.console.domains = .all 
            Database.log.console.level = .verbose
            let target_url: String? = args.get(name: "target_url")
            let replication_type: String? = args.get(name: "replication_type")
            let continuous: Bool? = args.get(name: "continuous")
            let authValue: AnyObject? = args.get(name: "authenticator")
            let authenticator: Authenticator? = authValue as? Authenticator
            let collections: [Collection]? = args.get(name: "collections")
            let collectionConfigurations: [CollectionConfiguration]? = args.get(name: "configuration")
            let headers: Dictionary<String, String>? = args.get(name: "headers")!
            let pinnedservercert: String? = args.get(name: "pinnedservercert")!
            let heartbeat: String? = args.get(name: "heartbeat")
            let maxRetries: String? = args.get(name: "max_retries")
            let maxRetryWaitTime: String? = args.get(name: "max_timeout")
            var replicatorType = ReplicatorType.pushAndPull
            if let type = replication_type {
                if type == "push" {
                    replicatorType = .push
                } else if type == "pull" {
                    replicatorType = .pull
                } else {
                    replicatorType = .pushAndPull
                }
            }
            var config: ReplicatorConfiguration
            var target: Endpoint?
            if (target_url != nil) {
                do {
                    try tryCatch {
                        target = URLEndpoint(url: URL(string: target_url!)!)
                    }
                } catch let error as NSError {
                    if let exception = error.userInfo["exception"] as? NSException {
                        if let reason = exception.reason {
                            throw RequestHandlerError.InvalidArgument("\(reason)")
                        } else {
                            throw RequestHandlerError.InvalidArgument("Unknown CBL Exception")
                        }
                    }
                }
            }
            #if COUCHBASE_ENTERPRISE
            let targetDatabase: Database? = args.get(name: "target_db")
            if (targetDatabase != nil) {
                target = DatabaseEndpoint(database: targetDatabase!)
                config = ReplicatorConfiguration(target: target!)
            }
            #endif

            if (target == nil) {
                throw RequestHandlerError.InvalidArgument("target url or database should be provided.")
            }
            config = ReplicatorConfiguration(target: target!)
            config.replicatorType = replicatorType
            config.authenticator = authenticator
            if continuous != nil {
                config.continuous = continuous!
            } else {
                config.continuous = false
            }
            if headers != nil {
                config.headers = headers
            }
            if pinnedservercert != nil {
                let path = Bundle(for: type(of:self)).path(forResource: pinnedservercert, ofType: "cer")
                let data = try! NSData(contentsOfFile: path!, options: [])
                let certificate = SecCertificateCreateWithData(nil, data)
                config.pinnedServerCertificate = certificate
            }
            if let heartbeat = heartbeat, let heartbeatDouble = Double(heartbeat) {
                config.heartbeat = heartbeatDouble
            }
            if let maxRetries = maxRetries, let maxRetryInInt = UInt(maxRetries) {
                config.maxAttempts = maxRetryInInt
            }
            
            if let maxRetryWaitTime = maxRetryWaitTime, let maxRetryWaitTimeDouble = Double(maxRetryWaitTime) {
                config.maxAttemptWaitTime = maxRetryWaitTimeDouble
            }
            if let auto_purge: String = args.get(name: "auto_purge") {
                config.enableAutoPurge = auto_purge.lowercased() == "enabled"
            }
            
            if let col = collections {
                if let colConfig = collectionConfigurations {
                    if colConfig.count == 1 {
                        config.addCollections(col, config: colConfig[0])
                    }
                    else {
                        assert(colConfig.count == col.count)
                        for (i, j) in colConfig.enumerated() {
                            config.addCollection(col[i], config: j)
                        }
                    }
                }
                else {
                    config.addCollections(col)
                }
            }
            return Replicator(config: config)
            
        case "replicatorConfiguration_configure":
            let source_db: Database? = args.get(name: "source_db")
            let target_url: String? = args.get(name: "target_url")
            let replication_type: String? = args.get(name: "replication_type")!
            let continuous: Bool? = args.get(name: "continuous")
            let channels: [String]? = args.get(name: "channels")
            let documentIDs: [String]? = args.get(name: "documentIDs")
            let authValue: AnyObject? = args.get(name: "authenticator")
            let authenticator: Authenticator? = authValue as? Authenticator
            let headers: Dictionary<String, String>? = args.get(name: "headers")!
            let pinnedservercert: String? = args.get(name: "pinnedservercert")!
            let pull_filter: Bool? = args.get(name: "pull_filter")!
            let push_filter: Bool? = args.get(name: "push_filter")!
            let filter_callback_func: String? = args.get(name: "filter_callback_func")
            let conflict_resolver: String? = args.get(name: "conflict_resolver")
            let heartbeat: String? = args.get(name: "heartbeat")
            let maxRetries: String? = args.get(name: "max_retries")
            let maxRetryWaitTime: String? = args.get(name: "max_timeout")
            
            var replicatorType = ReplicatorType.pushAndPull
            
            if let type = replication_type {
                if type == "push" {
                    replicatorType = .push
                } else if type == "pull" {
                    replicatorType = .pull
                } else {
                    replicatorType = .pushAndPull
                }
            }
            var config: ReplicatorConfiguration
            if (source_db == nil){
                throw RequestHandlerError.InvalidArgument("No source db provided")
            }

            var target: Endpoint?
            if (target_url != nil) {
                do {
                    try tryCatch {
                        target = URLEndpoint(url: URL(string: target_url!)!)
                    }
                } catch let error as NSError {
                    if let exception = error.userInfo["exception"] as? NSException {
                        if let reason = exception.reason {
                            throw RequestHandlerError.InvalidArgument("\(reason)")
                        } else {
                            throw RequestHandlerError.InvalidArgument("Unknown CBL Exception")
                        }
                    }
                }
            }
            
            #if COUCHBASE_ENTERPRISE
                let targetDatabase: Database? = args.get(name: "target_db")
                if (targetDatabase != nil) {
                    target = DatabaseEndpoint(database: targetDatabase!)
                    config = ReplicatorConfiguration(database: source_db!, target: target!)
                }
            #endif

            if (target == nil) {
                throw RequestHandlerError.InvalidArgument("target url or database should be provided.")
            }
            config = ReplicatorConfiguration(database: source_db!, target: target!)
            config.replicatorType = replicatorType
            if continuous != nil {
                config.continuous = continuous!
            } else {
                config.continuous = false
            }
            if headers != nil {
                config.headers = headers
            }
            config.authenticator = authenticator
            if channels != nil {
                config.channels = channels
            }
            if documentIDs != nil {
                config.documentIDs = documentIDs
            }
            if pinnedservercert != nil {
                let path = Bundle(for: type(of:self)).path(forResource: pinnedservercert, ofType: "cer")
                let data = try! NSData(contentsOfFile: path!, options: [])
                let certificate = SecCertificateCreateWithData(nil, data)
                config.pinnedServerCertificate = certificate
            }
            if pull_filter != false {
                if filter_callback_func == "boolean" {
                    config.pullFilter = _replicatorBooleanFilterCallback;
                } else if filter_callback_func == "deleted" {
                    config.pullFilter = _replicatorDeletedFilterCallback;
                } else if filter_callback_func == "access_revoked" {
                    config.pullFilter = _replicatorAccessRevokedCallback;
                } else {
                    config.pullFilter = _defaultReplicatorFilterCallback;
                }
            }
            if push_filter != false {
                if filter_callback_func == "boolean" {
                    config.pushFilter = _replicatorBooleanFilterCallback;
                } else if filter_callback_func == "deleted" {
                    config.pushFilter = _replicatorDeletedFilterCallback;
                } else if filter_callback_func == "access_revoked" {
                    config.pushFilter = _replicatorAccessRevokedCallback;
                } else {
                    config.pushFilter = _defaultReplicatorFilterCallback;
                }
            }
            switch conflict_resolver {
                case "local_wins":
                    config.conflictResolver = LocalWinCustomConflictResolver();
                    break
                case "remote_wins":
                    config.conflictResolver = RemoteWinCustomConflictResolver();
                    break;
                case "null":
                    config.conflictResolver = NullWinCustomConflictResolver();
                    break;
                case "merge":
                    config.conflictResolver = MergeWinCustomConflictResolver();
                    break;
                case "incorrect_doc_id":
                    config.conflictResolver = IncorrectDocIdCustomConflictResolver();
                    break;
                case "delayed_local_win":
                    config.conflictResolver = DelayedLocalWinCustomConflictResolver();
                    break;
                case "delete_not_win":
                    config.conflictResolver = DeleteDocCustomConflictResolver();
                    break
                case "exception_thrown":
                    config.conflictResolver = ExceptionThrownCustomConflictResolver();
                    break;
                default:
                    config.conflictResolver = ConflictResolver.default
                    break;
            }
            if let heartbeat = heartbeat, let heartbeatDouble = Double(heartbeat) {
                config.heartbeat = heartbeatDouble
            }
            if let maxRetries = maxRetries, let maxRetryInInt = UInt(maxRetries) {
                config.maxAttempts = maxRetryInInt
            }
            
            if let maxRetryWaitTime = maxRetryWaitTime, let maxRetryWaitTimeDouble = Double(maxRetryWaitTime) {
                config.maxAttemptWaitTime = maxRetryWaitTimeDouble
            }
            if let auto_purge: String = args.get(name: "auto_purge") {
                config.enableAutoPurge = auto_purge.lowercased() == "enabled"
            }
            return config

        case "replicatorConfiguration_getAuthenticator":
            let replicatorConfiguration: ReplicatorConfiguration = args.get(name: "configuration")!
            return replicatorConfiguration.authenticator
            
        case "replicatorConfiguration_getChannels":
            let replicatorConfiguration: ReplicatorConfiguration = args.get(name: "configuration")!
            return replicatorConfiguration.channels
            
        case "replicatorConfiguration_getDatabase":
            let replicatorConfiguration: ReplicatorConfiguration = args.get(name: "configuration")!
            return replicatorConfiguration.database
            
        case "replicatorConfiguration_getDocumentIDs":
            let replicatorConfiguration: ReplicatorConfiguration = args.get(name: "configuration")!
            return replicatorConfiguration.documentIDs
            
        case "replicatorConfiguration_getPinnedServerCertificate":
            let replicatorConfiguration: ReplicatorConfiguration = args.get(name: "configuration")!
            return replicatorConfiguration.pinnedServerCertificate
            
        case "replicatorConfiguration_getReplicatorType":
            let replicatorConfiguration: ReplicatorConfiguration = args.get(name: "configuration")!
            return replicatorConfiguration.replicatorType.rawValue
            
        case "replicatorConfiguration_getTarget":
            let replicatorConfiguration: ReplicatorConfiguration = args.get(name: "configuration")!
            return replicatorConfiguration.target
            
        case "replicatorConfiguration_isContinuous":
            let replicatorConfiguration: ReplicatorConfiguration = args.get(name: "configuration")!
            return replicatorConfiguration.continuous
        
        case "replicatorConfiguration_setAuthenticator":
            var replicatorConfiguration: ReplicatorConfiguration = args.get(name: "configuration")!
            let authenticator: Authenticator = args.get(name: "authenticator")!
            replicatorConfiguration.authenticator = authenticator
            return replicatorConfiguration
        
        case "replicatorConfiguration_setChannels":
            var replicatorConfiguration: ReplicatorConfiguration = args.get(name: "configuration")!
            let channels: [String] = args.get(name: "channels")!
            replicatorConfiguration.channels = channels
            return replicatorConfiguration
        
        case "replicatorConfiguration_setContinuous":
            var replicatorConfiguration: ReplicatorConfiguration = args.get(name: "configuration")!
            let continuous: Bool = args.get(name: "continuous")!
            replicatorConfiguration.continuous = continuous
            return replicatorConfiguration
        
        case "replicatorConfiguration_setDocumentIDs":
            var replicatorConfiguration: ReplicatorConfiguration = args.get(name: "configuration")!
            let documentIds: [String] = args.get(name: "documentIds")!
            replicatorConfiguration.documentIDs = documentIds
            return replicatorConfiguration
            
        case "replicatorConfiguration_setAutoPurge":
            var replicatorConfiguration: ReplicatorConfiguration = args.get(name: "configuration")!
            let auto_purge: Bool = args.get(name: "auto_purge")!
            replicatorConfiguration.enableAutoPurge = auto_purge
            return replicatorConfiguration

        case "replicatorConfiguration_setPinnedServerCertificate":
            var replicatorConfiguration: ReplicatorConfiguration = args.get(name: "configuration")!
            let cert: SecCertificate? = args.get(name: "cert")!
            replicatorConfiguration.pinnedServerCertificate = cert
            return replicatorConfiguration
        
        case "replicatorConfiguration_setReplicatorType":
            var replicatorConfiguration: ReplicatorConfiguration = args.get(name: "configuration")!
            let type: String = args.get(name: "replType")!
            var replicatorType: ReplicatorType
            switch (type) {
                case "push":
                    replicatorType = ReplicatorType.push
                    break
                case "pull":
                    replicatorType = ReplicatorType.pull
                    break
                default:
                    replicatorType = ReplicatorType.pushAndPull
            }
            replicatorConfiguration.replicatorType = replicatorType
            return replicatorConfiguration

        default:
            throw RequestHandlerError.MethodNotFound(method)
        }
        return ReplicatorConfigurationRequestHandler.VOID
    }


    private func _replicatorBooleanFilterCallback(document: Document, flags: DocumentFlags) -> Bool {
        let key:String = "new_field_1"
        if document.contains(key){
            let value:Bool = document.boolean(forKey: key)
            return value;
        }
        return true
    }
    
    private func _defaultReplicatorFilterCallback(document: Document, flags: DocumentFlags) -> Bool {
        return true;
    }
    
    private func _replicatorDeletedFilterCallback(document: Document, documentFlags: DocumentFlags) -> Bool {
        if (documentFlags.rawValue == 1) {
            return false
        }
        return true
    }
    
    private func _replicatorAccessRevokedCallback(document: Document, documentFlags: DocumentFlags) -> Bool {
        if (documentFlags.rawValue == 2) {
            return false
        }
        return true
    }
}

private class LocalWinCustomConflictResolver: ConflictResolverProtocol {
    func resolve(conflict: Conflict) -> Document? {
        let localDoc = conflict.localDocument
        let remoteDoc = conflict.remoteDocument
        if (localDoc == nil || remoteDoc == nil) {
            NSException(name: .internalInconsistencyException,
                        reason: "Either local doc or remote is/are null",
                        userInfo: nil).raise()
        }
        let docId = conflict.documentID
        checkMismatchDocId(localDoc: localDoc, remoteDoc: remoteDoc, docId: docId)
        return localDoc
    }
}

private class RemoteWinCustomConflictResolver: ConflictResolverProtocol{
    func resolve(conflict: Conflict) -> Document? {
        let localDoc = conflict.localDocument
        let remoteDoc = conflict.remoteDocument
        if (localDoc == nil || remoteDoc == nil) {
            NSException(name: .internalInconsistencyException,
                        reason: "Either local doc or remote is/are null",
                        userInfo: nil).raise()
        }
        let docId = conflict.documentID
        checkMismatchDocId(localDoc: localDoc, remoteDoc: remoteDoc, docId: docId)
        return remoteDoc
    }
}

private class NullWinCustomConflictResolver: ConflictResolverProtocol{
    func resolve(conflict: Conflict) -> Document? {
        let localDoc = conflict.localDocument
        let remoteDoc = conflict.remoteDocument
        if (localDoc == nil || remoteDoc == nil) {
            NSException(name: .internalInconsistencyException,
                        reason: "Either local doc or remote is/are null",
                        userInfo: nil).raise()
        }
        let docId = conflict.documentID
        checkMismatchDocId(localDoc: localDoc, remoteDoc: remoteDoc, docId: docId)
        return nil
    }
}

private class MergeWinCustomConflictResolver: ConflictResolverProtocol{
    func resolve(conflict: Conflict) -> Document? {
        /// Migrate the conflicted doc
        /// Algorithm creates a new doc with copying local doc and then adding any additional key
        /// from remote doc. Conflicting keys will have value from local doc.
        let localDoc = conflict.localDocument
        let remoteDoc = conflict.remoteDocument
        if (localDoc == nil || remoteDoc == nil) {
            NSException(name: .internalInconsistencyException,
                        reason: "Either local doc or remote is/are null",
                        userInfo: nil).raise()
        }
        let docId = conflict.documentID
        checkMismatchDocId(localDoc: localDoc, remoteDoc: remoteDoc, docId: docId)
        let newDoc = localDoc!.toMutable()
        let remoteDocMap = remoteDoc!.toDictionary()
        for (key, value) in remoteDocMap {
            if !newDoc.contains(key) {
                newDoc.setValue(value, forKey: key)
            }
        }
        return newDoc
    }
}

private class DelayedLocalWinCustomConflictResolver: ConflictResolverProtocol{
    func resolve(conflict: Conflict) -> Document? {
        let localDoc = conflict.localDocument
        let remoteDoc = conflict.remoteDocument
        if (localDoc == nil || remoteDoc == nil) {
            NSException(name: .internalInconsistencyException,
                        reason: "Either local doc or remote is/are null",
                        userInfo: nil).raise()
        }
        let docId = conflict.documentID
        checkMismatchDocId(localDoc: localDoc, remoteDoc: remoteDoc, docId: docId)
        sleep(10)
        return localDoc
    }
}

private class IncorrectDocIdCustomConflictResolver: ConflictResolverProtocol{
    func resolve(conflict: Conflict) -> Document? {
        let localDoc = conflict.localDocument
        let remoteDoc = conflict.remoteDocument
        if (localDoc == nil || remoteDoc == nil) {
            NSException(name: .internalInconsistencyException,
                        reason: "Either local doc or remote is/are null",
                        userInfo: nil).raise()
        }
        let docId = conflict.documentID
        checkMismatchDocId(localDoc: localDoc, remoteDoc: remoteDoc, docId: docId)
        let newId = "changed\(docId)"
        let newDoc = MutableDocument(id: newId, data: localDoc!.toDictionary())
        newDoc.setValue("couchbase", forKey: "new_value")
        return newDoc
    }
}

private class DeleteDocCustomConflictResolver: ConflictResolverProtocol{
    func resolve(conflict: Conflict) -> Document? {
        let localDoc = conflict.localDocument
        let remoteDoc = conflict.remoteDocument
        if (localDoc == nil || remoteDoc == nil) {
            NSException(name: .internalInconsistencyException,
                        reason: "Either local doc or remote is/are null",
                        userInfo: nil).raise()
        }
        let docId = conflict.documentID
        checkMismatchDocId(localDoc: localDoc, remoteDoc: remoteDoc, docId: docId)
        if remoteDoc == nil {
            return localDoc
        }
        return nil
    }
}

private class ExceptionThrownCustomConflictResolver: ConflictResolverProtocol{
    func resolve(conflict: Conflict) -> Document? {
        let localDoc = conflict.localDocument
        let remoteDoc = conflict.remoteDocument
        if (localDoc == nil || remoteDoc == nil) {
            NSException(name: .internalInconsistencyException,
                        reason: "Either local doc or remote is/are null",
                        userInfo: nil).raise()
        }
        let docId = conflict.documentID
        checkMismatchDocId(localDoc: localDoc, remoteDoc: remoteDoc, docId: docId)
        NSException(name: .internalInconsistencyException,
                    reason: "some exception happened inside custom conflict resolution",
                    userInfo: nil).raise()
        return remoteDoc
    }
}

private func checkMismatchDocId(localDoc: Document?, remoteDoc: Document?, docId: String) -> Void{
    if let remoteDocId = remoteDoc?.id {
        if remoteDocId != docId {
            NSException(name: .internalInconsistencyException,
                        reason: "DocId mismatch",
                        userInfo: nil).raise()
        }
    }
    if let localDocId = localDoc?.id {
        if localDocId != docId {
            NSException(name: .internalInconsistencyException,
                        reason: "DocId mismatch",
                        userInfo: nil).raise()
        }
    }
}
