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
    private func dataFromResource(name: String, ofType type: String) throws -> Data {
        let path = Bundle(for: Swift.type(of:self)).path(forResource: name, ofType: type)
        return try! NSData(contentsOfFile: path!, options: []) as Data
    }

    public func handleRequest(method: String, args: Args) throws -> Any? {
        
        switch method {
            
            /////////////////////////////
            // Peer to Peer Apis //
            ////////////////////////////
            
        case "peerToPeer_messageEndpointListenerStart":
            let database: Database = args.get(name:"database")!
            let port: Int = args.get(name:"port")!
            let peerToPeerListener: ReplicatorTcpListener = ReplicatorTcpListener(databases: [database], port: UInt32(port))
            peerToPeerListener.start()
            print("Server is getting started")
            return peerToPeerListener
       
        case "peerToPeer_serverStart":
            var peerToPeerListener: URLEndpointListener
            let database: Database = args.get(name:"database")!
            let config = URLEndpointListenerConfiguration.init(database: [database][0])
            let port: Int = args.get(name:"port")!
            let basicAuth: ListenerAuthenticator? = args.get(name: "basic_auth")!
            let disableTls: Bool? = args.get(name: "tls_disable")
            let tlsAuthenticator: Bool? = args.get(name: "tls_authenticator")!
            let tlsAuthType: String?  = args.get(name: "tls_auth_type")!
        
            if port > 0 {
                config.port = UInt16(port)
            }
            if disableTls != nil {
                config.disableTLS = disableTls!
            }
            
            if basicAuth != nil {
                print("Listener: Setting basic authentication")
                config.authenticator = basicAuth!
            }
            if (tlsAuthenticator != false) {
                let rootCertData = try! dataFromResource(name: "identity2/client-ca", ofType: "der")
                let rootCert = SecCertificateCreateWithData(kCFAllocatorDefault, rootCertData as CFData)!
                let listenerAuth = ListenerCertificateAuthenticator.init(rootCerts: [rootCert])
                config.authenticator = listenerAuth
                print("Client TLS self_signed_create")
            }
            if tlsAuthType == "self_signed" {
                let serverCertData = try! dataFromResource(name: "identity2/certs", ofType: "p12")
                try TLSIdentity.deleteIdentity(withLabel: "CBL-Server-Cert")
                let identity = try! TLSIdentity.importIdentity(withData: serverCertData, password: "123", label: "CBL-Server-Cert")
                config.tlsIdentity = identity
                print("TLS self_signed set")
            }
            if (tlsAuthType == "self_signed_create") {
                let attrs = [certAttrCommonName: "testkit"]
                try! TLSIdentity.deleteIdentity(withLabel: "CBL-Server-Cert")
                let identity = try! TLSIdentity.createIdentity(forServer: false, attributes: attrs, expiration: nil, label: "CBL-Server-Cert")
                config.tlsIdentity = identity
                print("TLS self_signed_create")
                
            }
            peerToPeerListener = URLEndpointListener.init(config: config)
            try peerToPeerListener.start()
            print("Url Listener Started")
            print(peerToPeerListener.urls)
            return peerToPeerListener

        case "peerToPeer_getListenerPort":
            let listener: URLEndpointListener = args.get(name:"listener")!
            print(listener.port)
            return listener.port

        case "peerToPeer_serverStop":
            let listenerType: String = args.get(name:"endPointType")!
            if listenerType == "MessageEndPoint" {
                let listener: ReplicatorTcpListener = args.get(name:"listener")!
                listener.stop()
            } else {
                let listener: URLEndpointListener = args.get(name:"listener")!
                listener.stop()
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
            var replicatorConfig: ReplicatorConfiguration
            var replicatorType = ReplicatorType.pushAndPull
            let basic_auth: Authenticator? = args.get(name: "basic_auth")!
            let tlsDisable: Bool? = args.get(name: "tls_disable")!
            let serverVerificationMode: Bool? = args.get(name: "server_verification_mode")!
            let tlsAuthType: String = args.get(name:"tls_auth_type")!
            let tlsAuthenticator: Bool? = args.get(name: "tls_authenticator")!
            var wsPort: String = "ws"
            
            if let type = replication_type {
                if type == "push" {
                    replicatorType = .push
                } else if type == "pull" {
                    replicatorType = .pull
                } else {
                    replicatorType = .pushAndPull
                }
            }

            if tlsDisable == false  {
                wsPort = "wss"
            }
            let url = URL(string: "\(wsPort)://\(host):\(port)/\(serverDBName)")!
            if endPointType == "URLEndPoint" {
                let urlEndPoint: URLEndpoint = URLEndpoint(url: url)
                replicatorConfig = ReplicatorConfiguration(database: database, target: urlEndPoint)
            } else{
                let endpoint = MessageEndpoint(uid: url.absoluteString, target: url, protocolType: ProtocolType.byteStream, delegate: self)
                replicatorConfig = ReplicatorConfiguration(database: database, target: endpoint)
            }
            if basic_auth != nil {
                replicatorConfig.authenticator = basic_auth
            }

            if serverVerificationMode != false {
                replicatorConfig.acceptOnlySelfSignedServerCertificate = true
                print("Replicator accept selfsigned Flag Set")
            }
            
            if (tlsAuthType == "self_signed") {
                let clientCertData = try! dataFromResource(name: "identity2/certs", ofType: "p12")
//                try! TLSIdentity.deleteIdentity(withLabel: "CBL-Client-Cert2")
                let identity = try! TLSIdentity.importIdentity(withData: clientCertData, password: "123", label: "CBL-Client-Cert2")
                replicatorConfig.pinnedServerCertificate = identity.certs[0]
                print("Replicator Pinned selfsigned Certs Set")
            }
            
            if (tlsAuthenticator != false) {
                let clientCertData = try dataFromResource(name: "identity2/client", ofType: "p12")
                try! TLSIdentity.deleteIdentity(withLabel: "CBL-Client-Cert")
                let identity = try! TLSIdentity.importIdentity(withData: clientCertData, password: "123", label: "CBL-Client-Cert")
                replicatorConfig.authenticator = ClientCertificateAuthenticator(identity: identity)
                print("Replicator Authenticator Set")
            }
            
            if continuous != nil {
                replicatorConfig.continuous = continuous!
            } else {
                replicatorConfig.continuous = false
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

        default:
            throw RequestHandlerError.MethodNotFound(method)
        }
        return PeerToPeerRequestHandler.VOID
    }
}

extension PeerToPeerRequestHandler: MessageEndpointDelegate {
    public func createConnection(endpoint: MessageEndpoint) -> MessageEndpointConnection {
        let url = endpoint.target as! URL
        return ReplicatorTcpClientConnection.init(url: url)
    }
}

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

