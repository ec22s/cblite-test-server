//
//  VectorSearchRequestHandler.swift
//  CBLTestServer-iOS
//
//  Created by Monty Ali on 13/02/2024.
//  Copyright © 2024 Raghu Sarangapani. All rights reserved.
//

import Foundation
import CouchbaseLiteSwift
import CoreML
import Tokenizers
import Hub
#if COUCHBASE_ENTERPRISE
public class VectorSearchRequestHandler {
    public func handleRequest(method: String, args: Args) async throws -> Any? {
        switch method {

        // create index on collection
        case "vectorSearch_createIndex":
            guard let database: Database = args.get(name: "database") else { throw RequestHandlerError.InvalidArgument("Invalid database argument")}

            let scopeName: String = (args.get(name: "scopeName")) ?? "_default"
            let collectionName: String = (args.get(name: "collectionName")) ?? "_default"

            guard let collection: Collection = try database.collection(name: collectionName, scope: scopeName) else { throw RequestHandlerError.IOException("Could not open specified collection")}

            guard let indexName: String = args.get(name: "indexName") else { throw RequestHandlerError.InvalidArgument("Invalid index name")}

            guard let expression: String = args.get(name: "expression") else { throw RequestHandlerError.InvalidArgument("Invalid expression for vector index")}

            // IndexConfiguration type is UInt32 but Serializer does not handle UInt currently so use Int for now and assume always pass +ve numbers
            // For manual testing, note that serialization of ints uses format Ixx where xx is an int value
            guard let dimensions: Int = args.get(name: "dimensions") else { throw RequestHandlerError.InvalidArgument("Invalid dimensions argument for vector index")}

            // As above for dimensions, UInt32 vs Int
            guard let centroids: Int = args.get(name: "centroids") else { throw RequestHandlerError.InvalidArgument("Invalid centroids argument for vector index")}

            let scalarEncoding: ScalarQuantizerType? = args.get(name: "scalarEncoding")
            // UInt32/Int problem
            // Note that subquantizers needs to be a factor of dimensions, runtime error if not
            // and bits b needs 4 <= b <= 12, maybe worth adding input validation in future
            let subquantizers: Int? = args.get(name: "subquantizers")
            let bits: Int? = args.get(name: "bits")
            let metric: String? = args.get(name: "metric")
            // UInt32/Int problem
            let minTrainingSize: Int? = args.get(name: "minTrainingSize")
            let maxTrainingSize: Int? = args.get(name: "maxTrainingSize")

            if scalarEncoding != nil && (bits != nil || subquantizers != nil) {
                throw RequestHandlerError.InvalidArgument("Cannot have scalar quantization and arguments for product quantization at the same time")
            }

            if (bits != nil && subquantizers == nil) || (bits == nil && subquantizers != nil) {
                throw RequestHandlerError.InvalidArgument("Product quantization requires both bits and subquantizers set")
            }

            var config = VectorIndexConfiguration(expression: expression, dimensions: UInt32(dimensions), centroids: UInt32(centroids))
            if let scalarEncoding {
                config.encoding = VectorEncoding.scalarQuantizer(type: scalarEncoding)
            }
            if let bits {
                config.encoding = VectorEncoding.productQuantizer(subquantizers: UInt32(subquantizers!), bits: UInt32(bits))
            }
            if let metric {
                switch metric {
                case "euclidean":
                    config.metric = DistanceMetric.euclidean
                case "cosine":
                    config.metric = DistanceMetric.cosine
                default:
                    throw RequestHandlerError.InvalidArgument("Invalid distance metric")
                }
            }
            if let minTrainingSize {
                config.minTrainingSize = UInt32(minTrainingSize)
            }
            if let maxTrainingSize {
                config.maxTrainingSize = UInt32(maxTrainingSize)
            }
            do {
                try collection.createIndex(withName: indexName, config: config)
                return "Created index with name \(indexName) on collection \(collectionName)"
            } catch {
                return "Could not create index: \(error)"
            }

        // returns the embedding for input string
        case "vectorSearch_getEmbedding":
            let model = vectorModel(key: "test")
            let testDic = MutableDictionaryObject()
            guard let input: String = args.get(name: "input") else { throw RequestHandlerError.InvalidArgument("Invalid input for prediction")}
            testDic.setValue(input, forKey: "test")
            let prediction = model.predict(input: testDic)
            let value = prediction?.array(forKey: "vector")
            if let value {
                return value.toArray()
            } else {
                return "Could not generate embedding"
            }

        // register model that creates embeddings on the field referred to by key
        case "vectorSearch_registerModel":
            guard let key: String = args.get(name: "key") else { throw RequestHandlerError.InvalidArgument("Invalid key")}
            guard let name: String = args.get(name: "name") else { throw RequestHandlerError.InvalidArgument("Invalid name")}
            let model = vectorModel(key: key)
            Database.prediction.registerModel(model, withName: name)
            return "Registered model with name \(name)"

        // this is a very bare bones sql++ query handler which expects the
        // user to pass the query as a string in the request body
        // and simply tries to create a query from that string
        // will fail for invalid query input
        // assumes that you already have created an index
        case "vectorSearch_query":
            // term is the search query that will be embedded and
            // queried against
            guard let term: String = args.get(name: "term") else { throw RequestHandlerError.InvalidArgument("Invalid search term")}

            // internal call to handler to get vector embedding for search term
            let embeddingArgs = Args()
            embeddingArgs.set(value: term, forName: "input")
            let embeddedTerm = try await self.handleRequest(method: "vectorSearch_getEmbedding", args: embeddingArgs)

            // the sql query, paramaterised by $vector which will be
            // the embedded term
            guard let sql: String = args.get(name: "sql") else { throw RequestHandlerError.InvalidArgument("Invalid sql string")}

            // database to execute query against
            // may change to handle collections
            guard let db: Database = args.get(name: "database") else { throw RequestHandlerError.InvalidArgument("Invalid database")}

            let params = Parameters()
            params.setValue(embeddedTerm, forName: "vector")
            let query = try db.createQuery(sql)
            query.parameters = params
            guard let queryResults = try? query.execute() else { throw RequestHandlerError.VectorPredictionError("Error executing SQL++ query")}

            var results: [[String:Any]] = []
            for result in queryResults {
                results.append(result.toDictionary())
            }

            return results

        // load pre generated vsTestDatabase
        case "vectorSearch_loadDatabase":
            let dbHandler = DatabaseRequestHandler()
            let preBuiltArgs = Args()
            preBuiltArgs.set(value: "Databases/vsTestDatabase.cblite2", forName: "dbPath")
            let dbPath = try dbHandler.handleRequest(method: "database_getPreBuiltDb", args: preBuiltArgs)
            preBuiltArgs.set(value: dbPath!, forName: "dbPath")
            preBuiltArgs.set(value: "vsTestDatabase", forName: "dbName")
            _ = try dbHandler.handleRequest(method: "database_copy", args: preBuiltArgs)
            let db: Database = try Database(name: "vsTestDatabase")
            return db
        // the handlers below are for manual testing purposes and do not
        // need to be replicated in the other test servers

        // get tokenized input
        case "vectorSearch_testTokenizer":
            guard let input: String = args.get(name: "input") else { throw RequestHandlerError.InvalidArgument("Invalid input")}
            return try tokenizeInput(input: input)

        // tokenize input then decode back to string, including padding
        case "vectorSearch_testDecode":
            guard let input: String = args.get(name: "input") else { throw RequestHandlerError.InvalidArgument("Invalid input")}
            let tokens = try tokenizeInput(input: input)
            let decoded = try decodeTokenIds(encoded: tokens)
            return ["tokens": tokens, "decoded": decoded]


        default:
            throw RequestHandlerError.MethodNotFound(method)
        }
    }

}

@available(iOS 16.0, *)
public class vectorModel: PredictiveModel {
    // key is the field name in the doc
    // to create embeddings on
    let key: String
    let model = try! float32_model()

    public func predict(input: DictionaryObject) -> DictionaryObject? {
        guard let text = input.string(forKey: self.key) else { return nil }
        let result = MutableDictionaryObject()
        do {
            let tokens = try tokenizeInput(input: text)
            let modelTokens = convertModelInputs(feature: tokens)
            let attentionMask = generateAttentionMask(tokenIdList: tokens)
            let modelMask = convertModelInputs(feature: attentionMask)
            let modelInput = float32_modelInput(input_ids: modelTokens, attention_mask: modelMask)
            let embedding = try model.prediction(input: modelInput)
            let vectorArray = castToDoubleArray(embedding.pooler_output)
            result.setValue(vectorArray, forKey: "vector")
            return result
        } catch {
            return nil
        }
    }

    init(key: String) {
        self.key = key
    }
}

// Reads a config from Files/ directory
func readConfig(name: String) throws -> Config? {
    if let url = Bundle.main.url(forResource: "Files/\(name)", withExtension: "json") {
        do {
            let data = try Data(contentsOf: url, options: .mappedIfSafe)
            let jsonResult = try JSONSerialization.jsonObject(with: data, options: .mutableLeaves)
            if let jsonDict = jsonResult as? [NSString: Any] {
                return Config(jsonDict)
            }
        } catch {
            throw RequestHandlerError.IOException("Error retrieving json config: \(error)")
        }
    }
    return nil
}

// might need validation on input size, gte-small model only
// takes up to 128 tokens, so need to truncate otherwise
func tokenizeInput(input: String) throws -> [Int] {
    guard let tokenizerConfig = try readConfig(name: "tokenizer_config") else { throw RequestHandlerError.IOException("Could not read config") }
    guard let tokenizerData = try readConfig(name: "tokenizer") else { throw RequestHandlerError.IOException("Could not read config") }
    let tokenizer = try! AutoTokenizer.from(tokenizerConfig: tokenizerConfig, tokenizerData: tokenizerData)
    let tokenized = tokenizer.encode(text: input)
    return padTokenizedInput(tokenIdList: tokenized)
}

// refactor tokenizer stuff
func decodeTokenIds(encoded: [Int]) throws -> String {
    guard let tokenizerConfig = try readConfig(name: "tokenizer_config") else { throw RequestHandlerError.IOException("Could not read config") }
    guard let tokenizerData = try readConfig(name: "tokenizer") else { throw RequestHandlerError.IOException("Could not read config") }
    let tokenizer = try! AutoTokenizer.from(tokenizerConfig: tokenizerConfig, tokenizerData: tokenizerData)
    let decoded = tokenizer.decode(tokens: encoded)
    return decoded
}

func padTokenizedInput(tokenIdList: [Int]) -> [Int] {
    // gte-small constants
    let modelInputLength = 128
    let padTokenId = 0
    let inputNumTokens = tokenIdList.count
    var paddedTokenList = [Int]()

    paddedTokenList += tokenIdList
    if inputNumTokens < modelInputLength {
        var numTokensToAdd = modelInputLength - inputNumTokens
        while numTokensToAdd > 0 {
            paddedTokenList.append(padTokenId)
            numTokensToAdd -= 1
        }
    }
    return paddedTokenList
}

func generateAttentionMask(tokenIdList: [Int]) -> [Int] {
    var mask = [Int]()
    for i in tokenIdList {
        if i == 0 {
            mask.append(0)
        } else {
            mask.append(1)
        }
    }
    return mask
}

// convert input tokens and attention mask to the correct shape
func convertModelInputs(feature: [Int]) -> MLMultiArray {
    let attentionMaskMultiArray = try? MLMultiArray(shape: [1, NSNumber(value: feature.count)], dataType: .int32)
    for i in 0..<feature.count {
        attentionMaskMultiArray?[i] = NSNumber(value: feature[i])
    }
    return attentionMaskMultiArray!
}

// https://stackoverflow.com/q/62887013
func castToDoubleArray(_ o: MLMultiArray) -> [Double] {
    var result: [Double] = Array(repeating: 0.0, count: o.count)
    for i in 0 ..< o.count {
        result[i] = o[i].doubleValue
    }
    return result
}

private extension VectorSearchRequestHandler {
    // internal function for copying documents to the test database
    func copyWordDocument(copyFrom: Collection, copyTo: Collection, docIds: [String]) throws {
        let copyToCollectionName = copyTo.name
        for docId in docIds {
            guard let doc = try copyFrom.document(id: docId) else { throw RequestHandlerError.IOException("Could not get document \(docId)")}
            var docData = doc.toDictionary()

            if copyToCollectionName == "indexVectors" {
                docData.removeValue(forKey: "vector")
            }

            let newDoc = MutableDocument(id: docId, data: docData)
            try copyTo.save(document: newDoc)
        }
    }
}
#endif