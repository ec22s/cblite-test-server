#include "VectorSearchMethods.h"
#include "MemoryMap.h"
#include "Router.h"
#include "Defines.h"
#include "Defer.hh"
#include "FleeceHelpers.h"

#include <iostream>
#include <fstream>
#include <vector>
#include <cstdio>

using namespace nlohmann;
using namespace std;
using namespace fleece;

#include INCLUDE_CBL(CouchbaseLite.h)

static FLMutableDict wordMap;
const string InMemoryDbName = "vsTestDatabase";

FLMutableDict getPrediction(FLDict input, string key) {
    const FLValue inputWord = FLDict_Get(input, flstr(key));
    FLMutableDict predictResult =  FLMutableDict_New();
    if (inputWord) {
        const FLValue embeddingsVector = FLDict_Get(wordMap, FLValue_AsString(inputWord));
        FLMutableDict_SetArray(predictResult, flstr("vector"), FLValue_AsArray(embeddingsVector));
    }
    return predictResult;
}

FLMutableDict predictFunction(void* context, FLDict input) {
    const FLValue inputWord = FLDict_Get(input, flstr("word"));
    auto embbedingsVector =  FLMutableArray_New();
    FLMutableDict predictResult =  FLMutableDict_New();
    DEFER {
        FLMutableArray_Release(embbedingsVector);
    };
    if (inputWord) {
        const FLValue tempVector = FLDict_Get(wordMap, FLValue_AsString(inputWord));
        FLArrayIterator iter;
        FLArrayIterator_Begin(FLValue_AsArray(tempVector), &iter);
        FLValue value;
        while (NULL != (value = FLArrayIterator_GetValue(&iter))) {
            FLMutableArray_AppendFloat(embbedingsVector, FLValue_AsFloat(value));
            FLArrayIterator_Next(&iter);
        }
    }
    if (embbedingsVector) {
        FLMutableDict_SetArray(predictResult, flstr("vector"), embbedingsVector);
    }
    return predictResult; 
}

static void CBLDatabase_EntryDelete(void* ptr) {
    CBLDatabase_Release(static_cast<CBLDatabase *>(ptr));
}


static FLMutableDict getWordMap() {
         std::string sql1 = "select word, vector from auxiliaryWords";
         std::string sql2 = "select word, vector from searchTerms";
         CBLError err;
         CBLDatabase* db;
         CBLQuery* query;
         CBLResultSet* rs; 
         FLMutableDict words = FLMutableDict_New();
         TRY(db = CBLDatabase_Open(flstr(InMemoryDbName), nullptr, &err), err);
         TRY(query = CBLDatabase_CreateQuery(db, kCBLN1QLLanguage, flstr(sql1), nullptr, &err), err);
         TRY(rs = CBLQuery_Execute(query, &err), err);
         while(CBLResultSet_Next(rs)) {
            FLValue word = CBLResultSet_ValueForKey(rs, flstr("word"));
            FLValue vector = CBLResultSet_ValueForKey(rs, flstr("vector"));
            if (vector) {
                FLMutableDict_SetArray(words, FLValue_AsString(word), FLValue_AsArray(vector));
            };

         }
         CBLQuery_Release(query);
         CBLResultSet_Release(rs);
         TRY(query = CBLDatabase_CreateQuery(db, kCBLN1QLLanguage, flstr(sql2), nullptr, &err), err);
         TRY(rs = CBLQuery_Execute(query, &err), err);
         while(CBLResultSet_Next(rs)) {
            FLValue word = CBLResultSet_ValueForKey(rs, flstr("word"));
            FLValue vector = CBLResultSet_ValueForKey(rs, flstr("vector"));
            if (vector) {
                FLMutableDict_SetArray(words, FLValue_AsString(word), FLValue_AsArray(vector));
            }
         }
         CBLQuery_Release(query);
         CBLResultSet_Release(rs);
         TRY(CBLDatabase_Close(db, &err), err);
         return words;
}

FLDict getEmbeddingDict(string term) {
    FLMutableDict testDict = FLMutableDict_New();
    FLMutableDict_SetString(testDict, flstr("word"), flstr(term));
    FLDict value = getPrediction(testDict, "word");
    FLMutableDict_Release(testDict);
    return value;
}

namespace vectorSearch_methods
{
    void vectorSearch_createIndex(json& body, mg_connection* conn)
    {
            const auto scopeName = body["scopeName"].get<string>();
            const auto collectionName = body["collectionName"].get<string>();
            const auto indexName = body["indexName"].get<string>();
            const auto expression = body["expression"].get<string>();
            const auto metric = body["metric"].get<string>();
            const auto dimensions = body["dimensions"].get<uint32_t>();
            const auto centroids = body["centroids"].get<uint32_t>();
            const auto minTrainingSize = body["minTrainingSize"].get<uint32_t>();
            const auto maxTrainingSize = body["maxTrainingSize"].get<uint32_t>();
            std::optional<uint32_t> bits;
            std::optional<uint32_t> subquantizers;
            std::optional<CBLScalarQuantizerType> scalarEncoding;
            CBLDistanceMetric dMetric;

            auto* encoding = CBLVectorEncoding_CreateNone();
            try
            {
                bits = static_cast<uint32_t>(body["bits"]);
                subquantizers = static_cast<uint32_t>(body["subquantizers"]);
            }
            catch (...)
            {
                bits.reset();
                subquantizers.reset();
            }

            try
            {
                scalarEncoding = static_cast<CBLScalarQuantizerType>(body["scalarEncoding"]);
            }
            catch (...)
            {
                scalarEncoding.reset();
            }

            if (scalarEncoding.has_value() && (bits.has_value() || subquantizers.has_value()))
            {
                throw std::invalid_argument("Cannot have scalar quantization and arguments for product quantization at the same time");
            }

            if ((bits.has_value() && !subquantizers.has_value()) || (!bits.has_value() && subquantizers.has_value()))
            {
                throw std::invalid_argument("Product quantization requires both bits and subquantizers set");
            }

            if (scalarEncoding.has_value())
            {
                encoding = static_cast<CBLVectorEncoding*>(CBLVectorEncoding_CreateScalarQuantizer(scalarEncoding.value()));
            }
            if (bits.has_value() && subquantizers.has_value())
            {
                encoding = static_cast<CBLVectorEncoding*>(CBLVectorEncoding_CreateProductQuantizer(subquantizers.value(), bits.value()));
            }

            if (metric == "euclidean")
            {
                dMetric = kCBLDistanceMetricEuclidean;
            }
            else if (metric == "cosine")
            {
                dMetric = kCBLDistanceMetricCosine;
            }
            else
            {
                throw std::invalid_argument("Invalid distance metric");
            }

            CBLVectorIndexConfiguration config = {};
            config.expression = flstr(expression);
            config.dimensions = dimensions;
            config.encoding = encoding;
            config.metric = dMetric;
            config.centroids = centroids;
            config.minTrainingSize = minTrainingSize;
            config.maxTrainingSize = maxTrainingSize;
            config.expressionLanguage = kCBLN1QLLanguage;
            with<CBLDatabase *>(body,"database", [conn, collectionName, scopeName, indexName, config](CBLDatabase* db)
            {
                CBLError err;
                CBLCollection* collection;
                TRY(collection = CBLDatabase_CreateCollection(db, flstr(collectionName),  flstr(scopeName), &err), err);
                TRY(CBLCollection_CreateVectorIndex(collection, flstr(indexName), config, &err), err);
                if (config.encoding) {
                    DEFER {   
                        CBLVectorEncoding_Free(config.encoding);
                    };
                }
                write_serialized_body(conn, "Created index with name " + indexName);
            });
    }

    void vectorSearch_loadDatabase(json& body, mg_connection* conn) {
        const auto dbPath = "Databases" + string(1, DIRECTORY_SEPARATOR) + InMemoryDbName  + ".cblite2";
        const auto dbName = InMemoryDbName;
        char cwd[1024];
        cbl_getcwd(cwd, 1024);
        const auto databasePath = string(cwd) + DIRECTORY_SEPARATOR + dbPath;
        const auto extensionsPath = string(cwd) + DIRECTORY_SEPARATOR + APP_EXTENSIONS_DIR;
        CBL_EnableVectorSearch(flstr(extensionsPath));
        CBLDatabaseConfiguration* databaseConfig = nullptr;
        CBLError err;
        CBLDatabase* db;
        TRY(CBL_CopyDatabase(flstr(databasePath), flstr(dbName), databaseConfig, &err), err);
        wordMap = getWordMap();
        TRY(db = CBLDatabase_Open(flstr(dbName), databaseConfig, &err), err);
        write_serialized_body(conn, memory_map::store(db, CBLDatabase_EntryDelete));

    }
   
   void vectorSearch_registerModel(json& body, mg_connection* conn) {
        const auto name = body["name"].get<string>();
        const auto key = body["key"].get<string>();

        CBLPredictiveModel model = {};
        model.context = nullptr;
        model.prediction = predictFunction;
        CBL_RegisterPredictiveModel(flstr(name), model);
        write_serialized_body(conn, "Model registered");
    }
    
    void vectorSearch_getEmbedding(json& body, mg_connection* conn)
    {
        auto embbedingsVector =  FLMutableArray_New();
        DEFER {
            FLMutableArray_Release(embbedingsVector);
        };
        auto vectorDict = getEmbeddingDict(body["input"].get<string>());
        DEFER {
            FLDict_Release(vectorDict);
        };
        FLValue embeddings = FLDict_Get(vectorDict, flstr("vector"));
        FLArrayIterator iter;
        FLArrayIterator_Begin(FLValue_AsArray(embeddings), &iter);
        FLValue value;
        while (NULL != (value = FLArrayIterator_GetValue(&iter))) {
            FLMutableArray_AppendFloat(embbedingsVector, FLValue_AsFloat(value));
            FLArrayIterator_Next(&iter);
        }
        write_serialized_body(conn, (FLValue)embbedingsVector);
    }


    void vectorSearch_query(json& body, mg_connection* conn) {

        with<CBLDatabase *>(body,"database", [conn, body](CBLDatabase* db)
            {
                auto embeddedTermDic = getEmbeddingDict(body["term"].get<string>());
                DEFER {
                   FLDict_Release(embeddedTermDic);
                };
                FLValue embeddedTerm = FLDict_Get(embeddedTermDic, flstr("vector"));
                auto sql = body["sql"].get<string>();
                json retVal = json::array();
                CBLError err;
                CBLQuery* query;

                TRY((query = CBLDatabase_CreateQuery(db, kCBLN1QLLanguage, flstr(sql), nullptr, &err)), err)
                DEFER {
                    CBLQuery_Release(query);
                };

                FLMutableDict qParam = FLMutableDict_New();
                FLMutableDict_SetArray(qParam, flstr("vector"), FLValue_AsArray(embeddedTerm));
                CBLQuery_SetParameters(query, FLDict(qParam));
                
                CBLResultSet* results;
                TRY(results = CBLQuery_Execute(query, &err), err);
                DEFER {
                    CBLResultSet_Release(results);
                };

                while(CBLResultSet_Next(results)) {
                    FLDict nextDict = CBLResultSet_ResultDict(results);
                    FLStringResult nextJSON = FLValue_ToJSON((FLValue)nextDict);
                    json next = json::parse(string((const char *)nextJSON.buf, nextJSON.size));
                    retVal.push_back(next);
                }
                FLMutableDict_Release(qParam);
                write_serialized_body(conn, retVal);
            });
    }

}
