//
//  PeerToPeerRequestHandler.swift
//  CBLTestServer-iOS
//
//  Created by Sridevi Saragadam on 7/23/18.
//  Copyright © 2018 Raghu Sarangapani. All rights reserved.
//
import Foundation
import CouchbaseLiteSwift

public class PeerToPeerRequestHandler {
    public static let VOID: String? = nil
    fileprivate var _pushPullReplListener:NSObjectProtocol?
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
    
    func dataFromResource(name: String, ofType type: String) throws -> Data {
        let path = Bundle(for: Swift.type(of:self)).path(forResource: name, ofType: type)
        return try! NSData(contentsOfFile: path!, options: []) as Data
    }

    public func handleRequest(method: String, args: Args) throws -> Any? {
        
        switch method {
            
            /////////////////////////////
            // Peer to Peer Apis //
            ////////////////////////////
        #if COUCHBASE_ENTERPRISE
        case "peerToPeer_messageEndpointListenerStart":
            let database: Database = args.get(name:"database")!
            let port: Int = args.get(name:"port")!
            let peerToPeerListener: ReplicatorTcpListener = ReplicatorTcpListener(databases: [database], port: UInt32(port))
            peerToPeerListener.start()
            return peerToPeerListener

        case "peerToPeer_getListenerPort":
            let listener: URLEndpointListener = args.get(name:"listener")!
            return listener.port
    
        case "peerToPeer_serverStart":
            var peerToPeerListener: URLEndpointListener?
            let database: Database = args.get(name:"database")!
            let wsPort: Int = args.get(name: "port")!
            let tls_disable: Bool? = args.get(name: "tls_disable")!
            var config = URLEndpointListenerConfiguration.init(database: [database][0])
            let tlsAuthType: String = args.get(name:"tls_auth_type")!
            let tlsAuthenticator: Bool? = args.get(name: "tls_authenticator")!
            let enable_delta_sync: Bool? = args.get(name: "enable_delta_sync")!

            config.port = UInt16(wsPort)
            config.disableTLS = tls_disable!
            
            if (tlsAuthenticator != false) {
                let rootCertData = try dataFromResource(name: "identity/client-ca", ofType: "der")
                let rootCert = SecCertificateCreateWithData(kCFAllocatorDefault, rootCertData as CFData)!
                let listenerAuth = ListenerCertificateAuthenticator.init(rootCerts: [rootCert])
                try! TLSIdentity.deleteIdentity(withLabel: "CBL-Cert")
                config.authenticator = listenerAuth
                print("========== Setting Authenticator ===========")
            }
            
            if tlsAuthType == "self_signed" {
                let serverCertData = try! dataFromResource(name: "identity/certs", ofType: "p12")
                try! TLSIdentity.deleteIdentity(withLabel: "CBL-Cert")
                let identity = try! TLSIdentity.importIdentity(withData: serverCertData, password: "123", label: "CBL-Cert")
                config.tlsIdentity = identity
                print("============= Setting identity ================")

            } else if tlsAuthType == "self_signed_create" {
                try! TLSIdentity.deleteIdentity(withLabel: "CBL-Cert")
                let id = try! TLSIdentity.createIdentity(forServer: true , attributes: [certAttrCommonName: "CBL-Server"], expiration: nil, label: "CBL-Cert")
                config.tlsIdentity = id
                print("========== Setting CreateIdentity =========")
            }

            if let auth: ListenerPasswordAuthenticator = args.get(name: "basic_auth") {
               config.authenticator = auth
            }

            config.enableDeltaSync = enable_delta_sync!
            peerToPeerListener = URLEndpointListener.init(config: config)
            try peerToPeerListener?.start()
            print("Server is getting started")
            return peerToPeerListener
            
        case "peerToPeer_serverStop":
            let type: String? = args.get(name:"endPointType")!
            if (type == "MessageEndPoint") {
                let listener_obj: ReplicatorTcpListener = args.get(name: "listener")!
                listener_obj.stop()
            } else {
                let listener_obj: URLEndpointListener = args.get(name: "listener")!
                listener_obj.stop()
            }
            
        case "peerToPeer_configure":
            let host: String = args.get(name:"host")!
            let port: Int = args.get(name:"port")!
            let serverDBName: String = args.get(name:"serverDBName")!
            let database: Database = args.get(name:"database")!
            let continuous: Bool? = args.get(name:"continuous")!
            let replication_type: String? = args.get(name: "replicationType")!
            let documentIDs: [String]? = args.get(name: "documentIDs")
            let endPointType: String = args.get(name: "endPointType")!
            let pull_filter: Bool? = args.get(name: "pull_filter")!
            let push_filter: Bool? = args.get(name: "push_filter")!
            let conflict_resolver: String? = args.get(name: "conflict_resolver")!
            let filter_callback_func: String? = args.get(name: "filter_callback_func")
            let auth: Authenticator? = args.get(name: "basic_auth")!
            let heartbeat: String? = args.get(name: "heartbeat")
            
            let serverVerificationMode: Bool? = args.get(name: "server_verification_mode")!
            let tls_disable: Bool? = args.get(name: "tls_disable")!
            let tlsAuthType: String? = args.get(name: "tls_auth_type")!
            let tlsAuthenticator: Bool? = args.get(name: "tls_authenticator")!
            var replicatorConfig: ReplicatorConfiguration
            var replicatorType = ReplicatorType.pushAndPull
            var wsPort: String?
            let maxRetries: String? = args.get(name: "max_retries")
            let maxRetryWaitTime: String? = args.get(name: "max_timeout")

            if let type = replication_type {
                if type == "push" {
                    replicatorType = .push
                } else if type == "pull" {
                    replicatorType = .pull
                } else {
                    replicatorType = .pushAndPull
                }
            }
            
            if tls_disable != true {
               wsPort = "wss"
            } else {
                wsPort = "ws"
            }

            let url = URL(string: "\(wsPort ?? "ws")://\(host):\(port)/\(serverDBName)")!
            if endPointType == "URLEndPoint"{
                let urlEndPoint: URLEndpoint = URLEndpoint(url: url)
                replicatorConfig = ReplicatorConfiguration(database: database, target: urlEndPoint)
            }
            else{
                let endpoint = MessageEndpoint(uid: url.absoluteString, target: url, protocolType: ProtocolType.byteStream, delegate: self)
                replicatorConfig = ReplicatorConfiguration(database: database, target: endpoint)
            }
            
            if auth != nil {
               replicatorConfig.authenticator = auth
            }
            if serverVerificationMode != false {
                replicatorConfig.acceptOnlySelfSignedServerCertificate = true
            }
            
            if tlsAuthType == "self_signed" {
                let certData = try! dataFromResource(name: "identity/certs", ofType: "p12")
                try! TLSIdentity.deleteIdentity(withLabel: "CBL-Cert")
                let identity = try! TLSIdentity.importIdentity(withData: certData, password: "123", label: "CBL-Cert")
                replicatorConfig.pinnedServerCertificate = identity.certs[0]
                print("======= pinned the certs to Replicator ============")
            }
            if tlsAuthenticator != false {
                let clientCertData = try dataFromResource(name: "identity/client", ofType: "p12")
                try! TLSIdentity.deleteIdentity(withLabel: "CBL-Cert")
                let identity = try TLSIdentity.importIdentity(withData: clientCertData, password: "123", label: "CBL-Cert")
                replicatorConfig.authenticator = ClientCertificateAuthenticator(identity: identity)
                print("====== Added Autheticator to Replicator ========")
            }

            if continuous != nil {
                replicatorConfig.continuous = continuous!
            }
            if documentIDs != nil {
                replicatorConfig.documentIDs = documentIDs
            }
            if pull_filter != false {
                if filter_callback_func == "boolean" {
                    replicatorConfig.pullFilter = _replicatorBooleanFilterCallback;
                } else if filter_callback_func == "deleted" {
                    replicatorConfig.pullFilter = _replicatorDeletedFilterCallback;
                } else if filter_callback_func == "access_revoked" {
                    replicatorConfig.pullFilter = _replicatorAccessRevokedCallback;
                } else {
                    replicatorConfig.pullFilter = _defaultReplicatorFilterCallback;
                }
            }
            if push_filter != false {
                if filter_callback_func == "boolean" {
                    replicatorConfig.pushFilter = _replicatorBooleanFilterCallback;
                } else if filter_callback_func == "deleted" {
                    replicatorConfig.pushFilter = _replicatorDeletedFilterCallback;
                } else if filter_callback_func == "access_revoked" {
                    replicatorConfig.pushFilter = _replicatorAccessRevokedCallback;
                } else {
                    replicatorConfig.pushFilter = _defaultReplicatorFilterCallback;
                }
            }
            switch conflict_resolver {
                case "local_wins":
                    replicatorConfig.conflictResolver = LocalWinCustomConflictResolver();
                    break
                case "remote_wins":
                    replicatorConfig.conflictResolver = RemoteWinCustomConflictResolver();
                    break;
                case "null":
                    replicatorConfig.conflictResolver = NullWinCustomConflictResolver();
                    break;
                case "merge":
                    replicatorConfig.conflictResolver = MergeWinCustomConflictResolver();
                    break;
                case "incorrect_doc_id":
                    replicatorConfig.conflictResolver = IncorrectDocIdCustomConflictResolver();
                    break;
                case "delayed_local_win":
                    replicatorConfig.conflictResolver = DelayedLocalWinCustomConflictResolver();
                    break;
                case "exception_thrown":
                    replicatorConfig.conflictResolver = ExceptionThrownCustomConflictResolver();
                    break;
                default:
                    replicatorConfig.conflictResolver = ConflictResolver.default
                    break;
            }
            if let heartbeat = heartbeat, let heartbeatDouble = Double(heartbeat) {
                replicatorConfig.heartbeat = heartbeatDouble
            }
            if let maxRetries = maxRetries, let maxRetryInInt = UInt(maxRetries) {
                replicatorConfig.maxAttempts = maxRetryInInt
            }
            if let maxRetryWaitTime = maxRetryWaitTime, let maxRetryWaitTimeDouble = Double(maxRetryWaitTime) {
                replicatorConfig.maxAttemptWaitTime = maxRetryWaitTimeDouble
            }
            replicatorConfig.replicatorType = replicatorType
            let replicator: Replicator = Replicator(config: replicatorConfig)
            return replicator

        case "peerToPeer_clientStart":
            let replicator: Replicator = args.get(name:"replicator")!
            replicator.start()
            print("Replicator has started")
            
        case "peerToPeer_addReplicatorEventChangeListener":
            let replication_obj: Replicator = args.get(name: "replicator")!
            let changeListener = MyDocumentReplicationListener()
            let listenerToken = replication_obj.addDocumentReplicationListener(changeListener.listener)
            changeListener.listenerToken = listenerToken
            return changeListener

        case "peerToPeer_removeReplicatorEventListener":
            let replication_obj: Replicator = args.get(name: "replicator")!
            let changeListener : MyDocumentReplicationListener = (args.get(name: "changeListener"))!
            replication_obj.removeChangeListener(withToken: changeListener.listenerToken!)

        case "peerToPeer_replicatorEventChangesCount":
            let changeListener: MyDocumentReplicationListener = (args.get(name: "changeListener"))!
            return changeListener.getChanges().count

        case "peerToPeer_replicatorEventGetChanges":
            let changeListener: MyDocumentReplicationListener = (args.get(name: "changeListener"))!
            let changes: [DocumentReplication] = changeListener.getChanges()
            var event_list: [String] = []
            for change in changes {
                for document in change.documents {
                    let doc_event:String = "doc_id: " + document.id
                    let error:String = ", error_code: " + document.error.debugDescription + ", error_domain: nil"
                    let flags:String = ", flags: " + document.flags.rawValue.description
                    let push:String = ", push: " + change.isPush.description
                    event_list.append(doc_event + error + push + flags)
                }
            }
            return event_list
        #endif
        default:
            throw RequestHandlerError.MethodNotFound(method)
        }
        return PeerToPeerRequestHandler.VOID
    }
}
#if COUCHBASE_ENTERPRISE
extension PeerToPeerRequestHandler: MessageEndpointDelegate {
    public func createConnection(endpoint: MessageEndpoint) -> MessageEndpointConnection {
        let url = endpoint.target as! URL
        return ReplicatorTcpClientConnection.init(url: url)
    }
}
#endif

private class LocalWinCustomConflictResolver: ConflictResolverProtocol {
    func resolve(conflict: Conflict) -> Document? {
        let localDoc = conflict.localDocument!
        let remoteDoc = conflict.remoteDocument!
        let docId = conflict.documentID
        checkMismatchDocId(localDoc: localDoc, remoteDoc: remoteDoc, docId: docId)
        return localDoc
    }
}

private class RemoteWinCustomConflictResolver: ConflictResolverProtocol{
    func resolve(conflict: Conflict) -> Document? {
        let localDoc = conflict.localDocument!
        let remoteDoc = conflict.remoteDocument!
        let docId = conflict.documentID
        checkMismatchDocId(localDoc: localDoc, remoteDoc: remoteDoc, docId: docId)
        return remoteDoc
    }
}

private class NullWinCustomConflictResolver: ConflictResolverProtocol{
    func resolve(conflict: Conflict) -> Document? {
        let localDoc = conflict.localDocument!
        let remoteDoc = conflict.remoteDocument!
        let docId = conflict.documentID
        checkMismatchDocId(localDoc: localDoc, remoteDoc: remoteDoc, docId: docId)
        return nil
    }
}

private class MergeWinCustomConflictResolver: ConflictResolverProtocol{
    func resolve(conflict: Conflict) -> Document? {
        let localDoc = conflict.localDocument!
        let remoteDoc = conflict.remoteDocument!
        let docId = conflict.documentID
        checkMismatchDocId(localDoc: localDoc, remoteDoc: remoteDoc, docId: docId)
        let newDoc = localDoc.toMutable()
        let remoteDocMap = remoteDoc.toDictionary()
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
        let localDoc = conflict.localDocument!
        let remoteDoc = conflict.remoteDocument!
        let docId = conflict.documentID
        checkMismatchDocId(localDoc: localDoc, remoteDoc: remoteDoc, docId: docId)
        sleep(10)
        return localDoc
    }
}

private class IncorrectDocIdCustomConflictResolver: ConflictResolverProtocol{
    func resolve(conflict: Conflict) -> Document? {
        let localDoc = conflict.localDocument!
        let remoteDoc = conflict.remoteDocument!
        let docId = conflict.documentID
        checkMismatchDocId(localDoc: localDoc, remoteDoc: remoteDoc, docId: docId)
        let newId = "changed\(docId)"
        let newDoc = MutableDocument(id: newId, data: localDoc.toDictionary())
        newDoc.setValue("_id", forKey: newId)
        return newDoc
    }
}

private class DeleteDocCustomConflictResolver: ConflictResolverProtocol{
    func resolve(conflict: Conflict) -> Document? {
        let localDoc = conflict.localDocument
        let remoteDoc = conflict.remoteDocument
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
        let docId = conflict.documentID
        checkMismatchDocId(localDoc: localDoc, remoteDoc: remoteDoc, docId: docId)
        NSException(name: .internalInconsistencyException,
                    reason: "some exception happened inside custom conflict resolution",
                    userInfo: nil).raise()
        return localDoc
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

