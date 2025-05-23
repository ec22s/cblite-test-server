//
//  Server.swift
//  CBLTestServer-iOS
//
//  Created by Raghu Sarangapani on 10/24/17.
//  Copyright © 2017 Raghu Sarangapani. All rights reserved.
//

import Foundation
import CouchbaseLiteSwift

enum ServerError: Error {
    case MethodNotImplemented(String)
}

enum RequestHandlerError: Error {
    case MethodNotFound(String)
    case InvalidArgument(String)
    case IOException(String)
    case VectorPredictionError(String)
}

enum ValueSerializerError: Error {
    case SerializerError(String)
    case DeSerializerError(String)
}

public class Server {
    let kPort:UInt = 8080
    let server: GCDWebServer!
    let dictionaryRequestHandler: DictionaryRequestHandler!
    let queryRequestHandler: QueryRequestHandler!
    let databaseRequestHandler: DatabaseRequestHandler!
    let documentRequestHandler: DocumentRequestHandler!
    let replicatorRequestHandler: ReplicatorRequestHandler!
    let collectionRequestHandler: CollectionRequestHandler!
    let scopeRequestHandler: ScopeRequestHandler!
    let arrayRequestHandler: ArrayRequestHandler!
    let sessionauthenticatorRequestHandler: SessionAuthenticatorRequestHandler!
    let encryptionkeyRequestHandler: EncryptionKeyRequestHandler!
    let blobRequestHandler: BlobRequestHandler!
    let datatypeRequestHandler: DataTypesInitiatorRequestHandler!
    let replicatorConfigurationRequestHandler: ReplicatorConfigurationRequestHandler!
    let expressionRequestHandler: ExpressionRequestHandler!
    let collationRequestHandler: CollationRequestHandler!
    let dataSourceRequestHandler: DataSourceRequestHandler!
    let functionRequestHandler: FunctionRequestHandler!
    let selectResultRequestHandler: SelectResultRequestHandler!
    let resultRequestHandler: ResultRequestHandler!
    let basicAuthenticatorRequestHandler: BasicAuthenticatorRequestHandler!
    let databaseConfigurationRequestHandler: DatabaseConfigurationRequestHandler!
    let peerToPeerRequestHandler: PeerToPeerRequestHandler!
    let listenerAuthenticatorRequestHandler: ListenerAuthenticatorRequestHandler!
    let predictiveQueryRequestHandler: PredictiveQueriesRequestHandler!
    let fileLoggingRequestHandler: FileLoggingRequestHandler!
    #if COUCHBASE_ENTERPRISE
    let vectorSearchRequestHandler: VectorSearchRequestHandler!
    #endif
    let memory = Memory()
    
    public init() {
        Database.log.console.level = .debug
        dictionaryRequestHandler = DictionaryRequestHandler()
        queryRequestHandler = QueryRequestHandler()
        databaseRequestHandler = DatabaseRequestHandler()
        documentRequestHandler = DocumentRequestHandler()
        replicatorRequestHandler = ReplicatorRequestHandler()
        arrayRequestHandler = ArrayRequestHandler()
        sessionauthenticatorRequestHandler = SessionAuthenticatorRequestHandler()
        encryptionkeyRequestHandler = EncryptionKeyRequestHandler()
        blobRequestHandler = BlobRequestHandler()
        datatypeRequestHandler = DataTypesInitiatorRequestHandler()
        replicatorConfigurationRequestHandler = ReplicatorConfigurationRequestHandler()
        expressionRequestHandler = ExpressionRequestHandler()
        collationRequestHandler = CollationRequestHandler()
        dataSourceRequestHandler = DataSourceRequestHandler()
        collectionRequestHandler = CollectionRequestHandler()
        scopeRequestHandler = ScopeRequestHandler()
        functionRequestHandler = FunctionRequestHandler()
        selectResultRequestHandler = SelectResultRequestHandler()
        resultRequestHandler = ResultRequestHandler()
        basicAuthenticatorRequestHandler = BasicAuthenticatorRequestHandler()
        databaseConfigurationRequestHandler = DatabaseConfigurationRequestHandler()
        peerToPeerRequestHandler = PeerToPeerRequestHandler()
        listenerAuthenticatorRequestHandler = ListenerAuthenticatorRequestHandler()
        predictiveQueryRequestHandler = PredictiveQueriesRequestHandler()
        fileLoggingRequestHandler = FileLoggingRequestHandler()
        #if COUCHBASE_ENTERPRISE
        vectorSearchRequestHandler = VectorSearchRequestHandler()
        #endif
        server = GCDWebServer()
        Database.log.console.level = LogLevel.verbose
        
        @Sendable func handlePostRequest (request: GCDWebServerRequest, completion: @escaping GCDWebServerCompletionBlock) async throws {
            var rawArgs = [String: Any]()
            
            var method = ""
            
            if request.path.hasPrefix("/") {
                let start = request.path.index(request.path.startIndex, offsetBy: 1)
                method = request.path.substring(from: start)
            } else {
                method = request.path
            }
            
            do {
                let args = Args()
                var queryParams = request.query
                let r = request as! GCDWebServerDataRequest

                if queryParams?.count == 0 {
                    queryParams = r.jsonObject as? Dictionary<String, AnyObject>
                }

                if let queryParams = queryParams {
                    // Get args from query params
                    for param in queryParams {
                        rawArgs[param.key as! String] = param.value

                        if let value = try ValueSerializer.deserialize(value:(param.value as? String), memory: self.memory) as Any? {
                            // Handle nil value
                            args.set(value: value, forName: param.key as! String)
                        } else {
                            args.set(value: "", forName: param.key as! String)
                        }
                    }
                }

                // Find and invoke the method on the RequestHandler.
                var body: Any? = nil
                var result: Any? = nil
                if "release" == method {
                    self.memory.remove(address: rawArgs["object"] as! String)
                } else if "flushMemory" == method {
                    self.memory.flushMemory()
                } else if method == "copy_files" {
                    result = self.memory.CopyFiles(args: args)
                    body = try ValueSerializer.serialize(value: result, memory: self.memory);
                } else{
                    if method.hasPrefix("query") {
                        result = try self.queryRequestHandler.handleRequest(method: method, args: args)
                    } else if method.hasPrefix("databaseConfiguration") {
                        result = try self.databaseConfigurationRequestHandler.handleRequest(method: method, args: args)
                    }else if method.hasPrefix("database") {
                        result = try self.databaseRequestHandler.handleRequest(method: method, args: args)
                    } else if method.hasPrefix("collection") {
                        result = try self.collectionRequestHandler.handleRequest(method: method, args: args)
                    } else if method.hasPrefix("scope") {
                        result = try self.scopeRequestHandler.handleRequest(method: method, args: args)
                    } else if method.hasPrefix("document") {
                        result = try self.documentRequestHandler.handleRequest(method: method, args: args)
                    } else if method.hasPrefix("dictionary") {
                        result = try self.dictionaryRequestHandler.handleRequest(method: method, args: args)
                    } else if method.hasPrefix("array") {
                        result = try self.arrayRequestHandler.handleRequest(method: method, args: args)
                    } else if method.hasPrefix("sessionAuthenticator") {
                        result = try self.sessionauthenticatorRequestHandler.handleRequest(method: method, args: args)
                    } else if method.hasPrefix("encryptionkey") {
                        result = try self.encryptionkeyRequestHandler.handleRequest(method: method, args: args)
                    } else if method.hasPrefix("blob") {
                        result = try self.blobRequestHandler.handleRequest(method: method, args: args)
                    } else if method.hasPrefix("datatype") {
                        result = try self.datatypeRequestHandler.handleRequest(method: method, args: args)
                    } else if method.hasPrefix("replicatorConfiguration") {
                        result = try self.replicatorConfigurationRequestHandler.handleRequest(method: method, args: args)
                    } else if method.hasPrefix("replicator") {
                        result = try self.replicatorRequestHandler.handleRequest(method: method, args: args)
                    } else if method.hasPrefix("expression") {
                        result = try self.expressionRequestHandler.handleRequest(method: method, args: args)
                    } else if method.hasPrefix("collation") {
                        result = try self.collationRequestHandler.handleRequest(method: method, args: args)
                    } else if method.hasPrefix("dataSource") {
                        result = try self.dataSourceRequestHandler.handleRequest(method: method, args: args)
                    } else if method.hasPrefix("function") {
                        result = try self.functionRequestHandler.handleRequest(method: method, args: args)
                    } else if method.hasPrefix("selectResult") {
                        result = try self.selectResultRequestHandler.handleRequest(method: method, args: args)
                    } else if method.hasPrefix("result") {
                        result = try self.resultRequestHandler.handleRequest(method: method, args: args)
                    } else if method.hasPrefix("basicAuthenticator") {
                        result = try self.basicAuthenticatorRequestHandler.handleRequest(method: method, args: args)
                    } else if method.hasPrefix("peerToPeer") {
                            result = try self.peerToPeerRequestHandler.handleRequest(method: method, args: args)
                    } else if method.hasPrefix("listenerAuthenticator") {
                            result = try self.listenerAuthenticatorRequestHandler.handleRequest(method: method, args: args)
                    } else if method.hasPrefix("predictiveQuery") {
                            result = try self.predictiveQueryRequestHandler.handleRequest(method: method, args: args)
                    } else if method.hasPrefix("logging") {
                        result = try self.fileLoggingRequestHandler.handleRequest(method: method, args: args)
                    } else if method.hasPrefix("vectorSearch") {
                        #if COUCHBASE_ENTERPRISE
                        let taskMethod = method
                        let taskResult = Task {
                            try await self.vectorSearchRequestHandler.handleRequest(method: taskMethod, args: args)
                        }
                        result = try await taskResult.value
                        #else
                        throw ServerError.MethodNotImplemented(method)
                        #endif
                    } else {
                        throw ServerError.MethodNotImplemented(method)
                    }
                    if result != nil {
                        body = try ValueSerializer.serialize(value: result, memory: self.memory);
                    }
                }

                if body != nil {
                    if body is RawData {
                        guard let dataObj = body as? RawData else {
                            fatalError("type should be a raw data")
                        }
                        completion(GCDWebServerDataResponse(data: dataObj.data, contentType: dataObj.contentType))
                    } else {
                        completion(GCDWebServerDataResponse(text: body as! String))
                    }
                } else {
                    // Send 200 code and close
                    completion(GCDWebServerDataResponse(text: "I-1"))
                }
            } catch let error as RequestHandlerError {
                var reason = "Unknown Request Handler Error"
                switch error {
                case .InvalidArgument(let r):
                    reason = r
                case .IOException(let r):
                    reason = r
                case .MethodNotFound(let r):
                    reason = r
                default:
                    break
                }
                let response = GCDWebServerDataResponse(text: reason)!
                response.statusCode = 432
                response.contentType = "text/plain"
                completion(response)
            } catch let error as NSError {
                let response = GCDWebServerDataResponse(text: error.localizedDescription)!
                completion(response)
            }
        }
        
        server.addDefaultHandler(forMethod: "POST", request: GCDWebServerDataRequest.self) {
            (request, completion) in
            Task {
                do {
                    try await handlePostRequest(request: request, completion: completion)
                } catch {
                    throw RequestHandlerError.IOException("Couldn't handle request")
                }
            }
        }
        #if COUCHBASE_ENTERPRISE
        do {
            try Extension.enableVectorSearch()
        }
        catch {
            print("The vetor search extension was not loaded")
        }
        #endif
        server.start(withPort: kPort, bonjourName: nil)
    }
}