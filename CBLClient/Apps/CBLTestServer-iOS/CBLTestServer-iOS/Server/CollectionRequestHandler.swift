//RequestHandler.swift
//CBLTestServer-iOS

import Foundation
import CouchbaseLiteSwift

public class CollectionRequestHandler {
    public func handleRequest(method: String, args: Args) throws -> Any? {
        switch method {
        case "collection_defaultCollection":
            let database: Database = (args.get(name:"database"))!
            let collection: Collection = try (database.defaultCollection())!
            return collection
            
        case "collection_getCollectionName":
            let collection: Collection = (args.get(name:"collection"))!
            return collection.name
            
        case "collection_createCollection":
            let scopeName: String = (args.get(name: "scopeName")) ?? "_default"
            let collectionName: String = (args.get(name: "collectionName"))!
            let database: Database = args.get(name: "database")!
            return try database.createCollection(name: collectionName, scope: scopeName)

            
        case "collection_collectionNames":
            let scope: String = (args.get(name:"scopeName")) ?? "_default"
            let database: Database = (args.get(name:"database"))!
            var names = [String]()
            let collectionNames = try database.collections(scope: scope)
            for collection in collectionNames{
                let collectionObject: Collection = collection
                names.append(collectionObject.name)
            }
            return names
            
        case "collection_collectionInstances":
            let scope: String = (args.get(name:"scopeName")) ?? "_default"
            let database: Database = (args.get(name:"database"))!
            return try database.collections(scope: scope)
        
        case "collection_deleteCollection":
            let scopeName: String = (args.get(name:"scopeName")) ?? "_default"
            let database: Database = (args.get(name:"database"))!
            let collectionName: String = (args.get(name:"collectionName"))!
            try database.deleteCollection(name: collectionName, scope: scopeName)
            
        case "collection_saveDocument":
            let document: MutableDocument = (args.get(name:"document"))!
            let collection: Collection = (args.get(name:"collection"))!
            try collection.save(document: document)

        case "collection_getDocument":
            let docId: String = (args.get(name: "docId"))!
            let collection : Collection = (args.get(name: "collection"))!
            return try collection.document(id: docId)
        
        case "collection_deleteDocument":
            let document: Document = (args.get(name: "document"))!
            let collection: Collection = (args.get(name: "collection"))!
            try collection.delete(document: document)

        case "collection_purgeDocument":
            let document: Document = (args.get(name: "document"))!
            let collection: Collection = (args.get(name: "collection"))!
            try collection.purge(document: document)
      
        case "collection_purgeDocumentID":
            let docID: String = (args.get(name: "docId"))!
            let collection: Collection = (args.get(name: "collection"))!
            try collection.purge(id: docID)

        case "collection_getDocumentExpiration":
            let docId: String = (args.get(name: "docId"))!
            let collection: Collection = (args.get(name: "collection"))!
            return try collection.getDocumentExpiration(id: docId)

        case "collection_setDocumentExpiration":
            let docId: String = (args.get(name: "docId"))!
            let expirationTime: Date = (args.get(name: "expirationTime"))!
            let collection: Collection = (args.get(name: "collection"))!
            try collection.setDocumentExpiration(id: docId, expiration: expirationTime)

        case "collection_getIndexNames":
            let collection: Collection = (args.get(name: "collection"))!
            return try collection.indexes()
            
        case "collection_deleteIndex":
            let collection: Collection = (args.get(name: "collection"))!
            let indexName: String = (args.get(name: "name"))!
            try collection.deleteIndex(forName: indexName)
            
        case "collection_createValueIndex":
            let collection: Collection = (args.get(name: "collection"))!
            let name: String = (args.get(name: "name"))!
            let expression: [String] = (args.get(name: "expression"))!
            let config = ValueIndexConfiguration(expression)
            try collection.createIndex(withName: name, config: config)          
            
        default:
            throw RequestHandlerError.MethodNotFound(method)
        }
        return nil
    }
}
