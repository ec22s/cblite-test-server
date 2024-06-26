#include "CollectionMethods.hpp"
#include "MemoryMap.h"
#include "Router.h"
#include "FleeceHelpers.h"
#include "DocumentMethods.h"
#include "Defines.h"
#include "Defer.hh"
extern "C" {
    #include "cdecode.h"
}

#include INCLUDE_CBL(CouchbaseLite.h)
using namespace std;
using namespace nlohmann;

void CBLCollection_EntryDelete (void* ptr)
{
    CBLCollection_Release(static_cast<CBLCollection*>(ptr));
}


void CBLCollectionDocument_EntryDelete(void* ptr)
{
    CBLDocument_Release(static_cast<CBLDocument*>(ptr));
}

void CBLCollectionScope_EntryDelete (void* ptr)
{
    CBLScope_Release(static_cast<CBLScope*>(ptr));
}

static void CBLCollection_DummyListener(void* context, const CBLCollectionChange* change) {

}

static void CBLCollection_DummyDocumentListener(void* context, const CBLDocumentChange* change) {
    
}

static void FLMutableDict_EntryDelete(void* ptr) {
    FLMutableDict_Release(static_cast<FLMutableDict>(ptr));
}

namespace collection_methods {
    //default collection object
    void collection_defaultCollection(json& body, mg_connection* conn){
        with<CBLDatabase *>(body, "database", [conn](CBLDatabase* db)
        {
            CBLError err = {};
            auto collection = CBLDatabase_DefaultCollection(db, &err);
            if(err.code!=0)
                write_serialized_body(conn, CBLError_Message(&err));
            else {
                write_serialized_body(conn, memory_map::store(collection,CBLCollection_EntryDelete));
            }
        });
    }

    //creating a collection in the scope
    void collection_createCollection(json& body, mg_connection* conn) {
        const auto collectionName = body["collectionName"].get<string>();
        const auto scopeName = body["scopeName"].get<string>();
        with<CBLDatabase *>(body,"database", [conn, &collectionName, &scopeName](CBLDatabase* db)
        {
            CBLError err = {};
            CBLCollection* newCollection = CBLDatabase_CreateCollection(db, flstr(collectionName),  flstr(scopeName), &err);
            if(err.code!=0)
                write_serialized_body(conn, CBLError_Message(&err));
            else
                write_serialized_body(conn, memory_map::store(newCollection, CBLCollection_EntryDelete));
        });
    }
    
    //document count for collection.
    void collection_documentCount(json &body, mg_connection* conn) {
        with<CBLCollection *>(body,"collection",[conn,&body](CBLCollection* collection){
            write_serialized_body(conn, CBLCollection_Count(collection));
        });
    }
    
    //collection name using collection object
    void collection_getCollectionName(json &body, mg_connection* conn) {
        with<CBLCollection *>(body, "collection",[conn,&body](CBLCollection* collection) {
            write_serialized_body(conn, CBLCollection_Name(collection));
        });
    }

    //delete collection in a scope
    void collection_deleteCollection(json& body, mg_connection* conn) {
        const auto scopeName = body["scopeName"].get<string>();
        const auto collectionName = body["collectionName"].get<string>();
        with<CBLDatabase *>(body,"database",[conn, &scopeName, &collectionName](CBLDatabase* db) {
            CBLError err = {};
            auto is_deleted = CBLDatabase_DeleteCollection(db, flstr(collectionName), flstr(scopeName), &err);
            if(err.code!=0)
                write_serialized_body(conn, CBLError_Message(&err));
            else
                write_serialized_body(conn, is_deleted);
        });
    }

    //names of all collection in a scope
    void collection_collectionNames(json& body, mg_connection* conn) {
        const auto scopeName = body["scopeName"].get<string>();
        with<CBLDatabase *>(body,"database",[conn,&scopeName](CBLDatabase* db) {
            CBLError err = {};
            FLMutableArray collectionNames = CBLDatabase_CollectionNames(db, flstr(scopeName), &err);
            if(err.code!=0)
            {
                write_serialized_body(conn, CBLError_Message(&err));
            }
            else
            {
                write_serialized_body(conn, reinterpret_cast<const FLValue>(collectionNames));
            }
        });
    }
    
    //returns collection object, function parameters are scope name and collection name
    void collection_collection(json& body, mg_connection* conn) {
        auto scopeName = body["scopeName"].get<string>();
        auto collectionName = body["collectionName"].get<string>();
        with<CBLDatabase *>(body,"database",[conn,&scopeName,&collectionName](CBLDatabase* db) {
            CBLError err = {};
            auto collection = CBLDatabase_Collection(db, flstr(collectionName), flstr(scopeName), &err);
            if(err.code!=0)
                write_serialized_body(conn, CBLError_Message(&err));
            else if(!collection)
                write_serialized_body(conn, NULL);
            else
                write_serialized_body(conn, memory_map::store(collection, CBLCollection_EntryDelete));
        });
    }

    //returns the scope of the collection
    void collection_collectionScope(json& body, mg_connection* conn) {
        with<CBLCollection *>(body,"collection",[conn](CBLCollection* collection) {
            write_serialized_body(conn, memory_map::store(CBLCollection_Scope(collection), CBLCollectionScope_EntryDelete));
        });
    }

    //returns document object, parameters are collection object, document Id
    void collection_getDocument(json& body, mg_connection* conn) {
        auto docId = body["docId"].get<string>();
        with<CBLCollection *>(body,"collection",[conn, &docId](CBLCollection* collection) {
            CBLError err = {};
            auto document = CBLCollection_GetDocument(collection, flstr(docId), &err);
            if(err.code!=0)
                write_serialized_body(conn, CBLError_Message(&err));
            else if(!document)
                write_serialized_body(conn, NULL);
            else
                write_serialized_body(conn, memory_map::store(document, CBLCollectionDocument_EntryDelete));
        });
    }

   void collection_getDocuments(json& body, mg_connection* conn) {
        const auto ids = body["ids"];
        FLMutableDict retVal = FLMutableDict_New();

        DEFER {
            FLMutableDict_Release(retVal);
        };

        with<CBLCollection *>(body, "collection", [retVal, &ids](CBLCollection* collection)
        {
            for(const auto& idJson : ids) {
                const auto id = idJson.get<string>();
                const CBLDocument* doc;
                CBLError err;
                TRY((doc = CBLCollection_GetDocument(collection, flstr(id), &err)), err);
                if(!doc) {
                    continue;
                }

                auto* slot = FLMutableDict_Set(retVal, flstr(id));
                FLSlot_SetValue(slot, reinterpret_cast<FLValue>(CBLDocument_Properties(doc)));
            }
        });

        write_serialized_body(conn, reinterpret_cast<FLValue>(retVal));
   }

    //save document to the collection, parameters are collection object, document object and error object
    void collection_saveDocument(json& body, mg_connection* conn) {
        with<CBLCollection *>(body, "collection", [conn, &body](CBLCollection* collection) {
            with<CBLDocument *>(body, "document", [conn, collection](CBLDocument* document) {
                CBLError err;
                CBLCollection_SaveDocument(collection, document, &err);
                if(err.code!=0)
                    write_serialized_body(conn,CBLError_Message(&err));
                else
                    write_empty_body(conn);
            });
        });
    }

    //save document in collection, params are collection object, document object, concurrency control object and error object
    void collection_saveDocumentWithConcurrencyControl(json& body, mg_connection* conn) {
        string concurrencyControlType;
        if(body.contains("concurrencyControlType")) {
            concurrencyControlType = body["concurrencyControlType"].get<string>();
        }
    
        with<CBLCollection *>(body, "collection", [conn, &body, &concurrencyControlType](CBLCollection* collection) {
            with<CBLDocument *>(body, "document", [conn, collection, &concurrencyControlType](CBLDocument* d) {
                auto concurrencyType = kCBLConcurrencyControlLastWriteWins;
                if(concurrencyControlType == "failOnConflict") {
                    concurrencyType = kCBLConcurrencyControlFailOnConflict;
                }
                CBLError err {};
                if(!CBLCollection_SaveDocumentWithConcurrencyControl(collection, d, concurrencyType, &err) && err.code != (int)kCBLErrorConflict) {
                    write_serialized_body(conn, CBLError_Message(&err));
                }
            });
        });
        write_empty_body(conn);
    }

    //delete document in collection, params are collection object, document object and err object
    void collection_deleteDocument(json& body, mg_connection* conn) {
        with<CBLCollection *>(body, "collection", [conn, &body](CBLCollection* collection) {
            with<CBLDocument *>(body, "document", [conn, collection] (CBLDocument* d) {
                CBLError err = {};
                CBLCollection_DeleteDocument(collection, d, &err);
                if(err.code!=0)
                    write_serialized_body(conn, CBLError_Message(&err));
                else
                    write_empty_body(conn);
            });
        });
    }

     void collection_updateDocument(json& body, mg_connection* conn) {
        const auto id = body["id"].get<string>();
        const auto data = body["data"];
        with<CBLCollection *>(body, "collection", [&id, &data](CBLCollection* collection)
        {
            CBLDocument* doc;
            CBLError err;
            TRY(doc = CBLCollection_GetMutableDocument(collection, flstr(id), &err), err)
            DEFER {
                CBLDocument_Release(doc);
            };

            FLMutableDict newContent = FLMutableDict_New();
            for(auto& [key, value] : data.items()) {
                writeFleece(newContent, key, value);
            }

            CBLDocument_SetProperties(doc, newContent);
            FLMutableDict_Release(newContent);
            TRY(CBLCollection_SaveDocument(collection, doc, &err), err)
        });

        write_empty_body(conn);
    }

    //delete document in collection with concurrency control
    void collection_deleteDocumentWithConcurrencyControl(json& body, mg_connection* conn) {
        string concurrencyControlType;
        if(body.contains("concurrencyControlType")) {
            concurrencyControlType = body["concurrencyControlType"].get<string>();
        }
    
        with<CBLCollection *>(body, "collection", [conn, &body, &concurrencyControlType](CBLCollection* collection) {
            with<CBLDocument *>(body, "document", [conn, collection, &concurrencyControlType](CBLDocument* d) {
                auto concurrencyType = kCBLConcurrencyControlLastWriteWins;
                if(concurrencyControlType == "failOnConflict") {
                    concurrencyType = kCBLConcurrencyControlFailOnConflict;
                }
                CBLError err {};
                if(!CBLCollection_DeleteDocumentWithConcurrencyControl(collection, d, concurrencyType, &err) && err.code != (int)kCBLErrorConflict) {
                    write_serialized_body(conn, CBLError_Message(&err));
                }
            });
        });
        write_empty_body(conn);
    }

    //purge document in collection
    void collection_purgeDocument(json& body, mg_connection* conn) {
        with<CBLCollection *>(body, "collection", [conn, &body](CBLCollection* collection) {
            with<CBLDocument *>(body, "document", [conn, collection](CBLDocument* d) {
                CBLError err = {};
                auto purge = CBLCollection_PurgeDocument(collection, d, &err);
                if(err.code!=0)
                {
                    write_serialized_body(conn, CBLError_Message(&err));
                }
                else
                    write_serialized_body(conn, purge);
            });
        });
    }

    //purge document by Id
    void collection_purgeDocumentID(json& body, mg_connection* conn) {
        const auto docID = body["docId"].get<string>();
        with<CBLCollection *>(body, "collection", [conn, &docID](CBLCollection* collection) {
                CBLError err = {};
                auto purge = CBLCollection_PurgeDocumentByID(collection, flstr(docID), &err);
                if(err.code!=0)
                {
                    write_serialized_body(conn, CBLError_Message(&err));
                }
                else
                    write_serialized_body(conn, purge);
            });
    }

    //Document expiration time and date
    void collection_getDocumentExpiration(json& body, mg_connection* conn) {
        with<CBLCollection *>(body, "collection", [conn,&body](CBLCollection* collection) {
            const auto docId = body["docId"].get<string>();
            CBLError err = {};
            const auto expirateDate = CBLCollection_GetDocumentExpiration(collection, flstr(docId), &err);
            if(err.code!=0)
                write_serialized_body(conn, CBLError_Message(&err));
            else {
                write_serialized_body(conn, CBLTimestamp(expirateDate));
            }
        });
    }

    //set document expiration
    void collection_setDocumentExpiration(json& body, mg_connection* conn) {
        with<CBLCollection *>(body, "collection", [conn, &body](CBLCollection* collection) {
            const auto docId = body["docId"].get<string>();
            const auto expiration = body["expiration"];
            CBLError err = {};
            CBLCollection_SetDocumentExpiration(collection, flstr(docId), CBLTimestamp(expiration), &err);
            if(err.code!=0)
                write_serialized_body(conn, CBLError_Message(&err));
            else
                write_empty_body(conn);
        });
    }

    //get mutuable document from the collection
    void collection_getMutableDocument(json& body, mg_connection* conn){
        with<CBLCollection *>(body, "collection", [conn, &body](CBLCollection* collection) {
            const auto docId = body["docId"].get<string>();
            CBLError err = {};
            const auto document = CBLCollection_GetMutableDocument(collection, flstr(docId), &err);
            if(err.code!=0)
                write_serialized_body(conn, CBLError_Message(&err));
            else
                write_serialized_body(conn, memory_map::store(document, CBLCollectionDocument_EntryDelete));
        });
    }

    //create value index
    void collection_createValueIndex(json& body, mg_connection* conn) {
        with<CBLCollection *>(body, "collection", [conn, &body](CBLCollection* collection) {
            CBLValueIndexConfiguration config = {};
            config.expressionLanguage = kCBLN1QLLanguage;
            config.expressions = flstr(body["expression"].get<string>());
            const auto name = body["name"].get<string>();
            CBLError err = {};
            bool createValueIndex = CBLCollection_CreateValueIndex(collection, flstr(name), config , &err);
            if(err.code!=0)
                write_serialized_body(conn, CBLError_Message(&err));
            else
                write_serialized_body(conn, createValueIndex);
        });
    }

    //create full text index
    void collection_createFullTextIndex(json& body, mg_connection* conn) {
        with<CBLCollection *>(body, "collection", [conn,&body](CBLCollection* collection) {
            const auto name = flstr(body["name"].get<string>());
            CBLError err = {};
            CBLFullTextIndexConfiguration config = {};
            config.expressionLanguage = kCBLN1QLLanguage;
            config.expressions = flstr(body["expression"].get<string>());
            config.language = flstr(body["language"].get<string>());
            if(body["ignoreAccents"] == true)
                config.ignoreAccents = true;
            bool createFullTextIndex = CBLCollection_CreateFullTextIndex(collection, name, config, &err);
            if(err.code!=0)
                write_serialized_body(conn, CBLError_Message(&err));
            else
                write_serialized_body(conn, createFullTextIndex);
        });
    }

    //delete index
    void collection_deleteIndex(json& body, mg_connection* conn) {
        with<CBLCollection *>(body, "collection", [conn, &body](CBLCollection* collection) {
            auto name = body["name"].get<string>();
            CBLError err = {};
            auto deleteIndex = CBLCollection_DeleteIndex(collection, flstr(name), &err);
            if(err.code!=0)
                write_serialized_body(conn, CBLError_Message(&err));
            else
                write_serialized_body(conn, deleteIndex);
        });
    }

    //get index names
    void collection_getIndexNames(json& body, mg_connection* conn) {
        with<CBLCollection *>(body, "collection", [conn, &body](CBLCollection* collection) {
            CBLError err = {};
            FLMutableArray index_names = CBLCollection_GetIndexNames(collection, &err);
            if(err.code!=0)
                write_serialized_body(conn, CBLError_Message(&err));
            else {
                write_serialized_body(conn, reinterpret_cast<const FLValue>(index_names));
            }
        });
    }

    //add change listener
    void collection_addChangeListener(json& body, mg_connection* conn) {
        with<CBLCollection *>(body, "collection", [conn](CBLCollection* collection) {
            CBLListenerToken* token = CBLCollection_AddChangeListener(collection, CBLCollection_DummyListener, nullptr);
            write_serialized_body(conn, memory_map::store(token, nullptr));
        });
    }

    //add document change listener
    void collection_addDocumentChangeListener(json& body, mg_connection* conn) {
        with<CBLCollection *>(body, "collection", [conn,&body](CBLCollection* collection) {
            const auto docId = body["docId"].get<string>();
            CBLListenerToken* token = CBLCollection_AddDocumentChangeListener(collection, flstr(docId), CBLCollection_DummyDocumentListener, nullptr);
            write_serialized_body(conn, memory_map::store(token, nullptr));
        });
    }
    
    void collection_saveDocuments(json& body, mg_connection* conn) {
    const auto docDict = body["documents"];
        with<CBLDatabase *>(body, "database", [&body, &docDict](CBLDatabase* db){
            with<CBLCollection *>(body, "collection", [&db, &docDict](CBLCollection* collection)
            {
                CBLError err;
                TRY(CBLDatabase_BeginTransaction(db, &err), err)

                bool success = false;
                DEFER {
                    TRY(CBLDatabase_EndTransaction(db, success, &err), err)
                };

                for(auto& [ key, value ] : docDict.items()) {
                    auto docBody = value;
                    docBody.erase("_id");
                    auto* doc = CBLDocument_CreateWithID(flstr(key));
                    auto* body = FLMutableDict_New();
                    for (const auto& [ bodyKey, bodyValue ] : docBody.items()) {
                        if(bodyKey == "_attachments") {
                            for (const auto& [ blobKey, blobValue ] : bodyValue.items()) {
                                base64_decodestate state;
                                base64_init_decodestate(&state);
                                auto b64 = blobValue["data"].get<string>();
                                auto tmp = malloc(b64.size());
                                size_t size = base64_decode_block((const uint8_t *)b64.c_str(), b64.size(), tmp, &state);
                                auto blobContent = FLSliceResult_CreateWith(tmp, size);
                                auto blob = CBLBlob_CreateWithData(FLSTR("application/octet-stream"), FLSliceResult_AsSlice(blobContent));
                                auto slot = FLMutableDict_Set(body, flstr(blobKey));
                                FLSlot_SetBlob(slot, blob);
                                FLSliceResult_Release(blobContent);
                            }
                        }
                        else {
                            writeFleece(body, bodyKey, bodyValue);
                        }
                    }

                    CBLDocument_SetProperties(doc, body);
                    FLMutableDict_Release(body);
                    TRY(CBLCollection_SaveDocument(collection, doc, &err), err);
                }
                success = true;
            });
         });
        write_empty_body(conn);
    }
}
