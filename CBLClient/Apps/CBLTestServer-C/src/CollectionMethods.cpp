#include "CollectionMethods.hpp"
#include "Defines.h"
#include "MemoryMap.h"
#include "Router.h"
#include "FleeceHelpers.h"
#include "DocumentMethods.h"

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

namespace collection_methods {
    //default scope object
    void collection_defaultScope(json& body, mg_connection* conn){
        with<CBLDatabase *>(body, "database", [conn](CBLDatabase* db)
        {
            CBLError* err = new CBLError();
            CBLScope* scope = CBLDatabase_DefaultScope(db, err);
            if(err->code!=0)
                write_serialized_body(conn, err->code);
            else {
                write_serialized_body(conn, memory_map::store(scope, CBLCollectionScope_EntryDelete));
            }
        });
    }
    
    //default collection object
    void collection_defaultCollection(json& body, mg_connection* conn){
        with<CBLDatabase *>(body, "database", [conn](CBLDatabase* db)
        {
            CBLError* err = new CBLError();
            CBLCollection* collection = CBLDatabase_DefaultCollection(db, err);
            if(err->code!=0)
                write_serialized_body(conn, err->code);
            else {
                write_serialized_body(conn, memory_map::store(collection,CBLCollection_EntryDelete));
            }
        });
    }

    //creating a collection in the scope
    void collection_createCollection(json& body, mg_connection* conn) {
        const auto scopeName = body["scopeName"].get<string>();
        const auto collectionName = body["collectionName"].get<string>();
        with<CBLDatabase *>(body,"database", [conn, &collectionName, &scopeName](CBLDatabase* db)
        {
            CBLError* err = new CBLError();
            CBLCollection* newCollection = CBLDatabase_CreateCollection(db, flstr(collectionName),  flstr(scopeName), err);
            if(err->code!=0)
                write_serialized_body(conn, err->code);
            else
                write_serialized_body(conn, memory_map::store(newCollection, CBLCollection_EntryDelete));
        });
    }
    
    //document count for collection.
    void collection_documentCount(json &body, mg_connection* conn) {
        with<CBLCollection *>(body,"collectionName",[conn,&body](CBLCollection* collectionName){
            write_serialized_body(conn, CBLCollection_Count(collectionName));
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
            CBLError* err = new CBLError();
            auto is_deleted = CBLDatabase_DeleteCollection(db, flstr(collectionName), flstr(scopeName), err);
            if(err->code!=0)
                write_serialized_body(conn, err->code);
            else
                write_serialized_body(conn, is_deleted);
        });
    }

    //names of all collection in a scope
    void collection_collectionNames(json& body, mg_connection* conn) {
        const auto scopeName = body["scopeName"].get<string>();
        with<CBLDatabase *>(body,"database",[conn,&scopeName](CBLDatabase* db) {
            json keys(0, nullptr);
            CBLError* err = new CBLError();
            FLMutableArray collectionNames = CBLDatabase_CollectionNames(db, flstr(scopeName), err);
            if(err->code!=0)
            {
                write_serialized_body(conn, err->code);
            }
            else
            {
                FLArrayIterator i;
                FLArrayIterator_Begin(collectionNames, &i);
                do {
                    const auto collectionName = FLArrayIterator_GetValue(&i);
                    FLStringResult jsonString = FLValue_ToString(collectionName);
                    keys.push_back(jsonString);
                } while(FLArrayIterator_Next(&i));
                write_serialized_body(conn, keys);

            }
        });
    }

    //names of all scopes in the database
    void collection_allScope(json& body, mg_connection* conn) {
        with<CBLDatabase *>(body,"database",[conn](CBLDatabase* db){
            json scopes(0, nullptr);
            CBLError* err = new CBLError();
            auto scopeNames = CBLDatabase_ScopeNames(db, err);
            if(err->code!=0)
                write_serialized_body(conn, err->code);
            else {
                FLArrayIterator i;
                FLArrayIterator_Begin(scopeNames, &i);
                do {
                    const auto scopename = FLArrayIterator_GetValue(&i);
                    FLStringResult jsonString = FLValue_ToString(scopename);
                    scopes.push_back(jsonString);
                } while(FLArrayIterator_Next(&i));
                write_serialized_body(conn, scopes);
            }
        });
    }
    
    //returns existing scope object with the given name
    void collection_scope(json& body, mg_connection* conn) {
        auto scopeName = body["scopeName"].get<string>();
        with<CBLDatabase *>(body,"database",[conn,&scopeName](CBLDatabase* db){
            CBLError* err = new CBLError();
            auto scope = CBLDatabase_Scope(db, flstr(scopeName), err);
            if(err->code!=0)
                write_serialized_body(conn, err->code);
            else
                write_serialized_body(conn, memory_map::store(scope, CBLCollectionScope_EntryDelete));
        });
    }
    
    //returns collection object, function parameters are scope name and collection name
    void collection_collection(json& body, mg_connection* conn) {
        auto scopeName = body["scopeName"].get<string>();
        auto collectionName = body["collectionName"].get<string>();
        with<CBLDatabase *>(body,"database",[conn,&scopeName,&collectionName](CBLDatabase* db) {
            CBLError* err = new CBLError();
            auto collection = CBLDatabase_Collection(db, flstr(collectionName), flstr(scopeName), err);
            if(err->code!=0)
                write_serialized_body(conn, err->code);
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
            CBLError* err = new CBLError();
            auto document = CBLCollection_GetDocument(collection, flstr(docId), err);
            if(err->code!=0)
                write_serialized_body(conn, err->code);
            else
                write_serialized_body(conn, memory_map::store(document, CBLCollectionDocument_EntryDelete));
        });
    }

    //save document to the collection, parameters are collection object, document object and error object
    void collection_saveDocument(json& body, mg_connection* conn) {
        with<CBLCollection *>(body, "collection", [conn, &body](CBLCollection* collection) {
            with<CBLDocument *>(body, "document", [conn, collection](CBLDocument* document) {
                CBLError err;
                TRY(CBLCollection_SaveDocument(collection, document, &err), err);
            });
        });
        write_empty_body(conn);
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
                    string errMsg = to_string(CBLError_Message(&err));
                    throw domain_error(errMsg);
                }
            });
        });
        write_empty_body(conn);
    }

    //delete document in collection, params are collection object, document object and err object
    void collection_deleteDocument(json& body, mg_connection* conn) {
        with<CBLCollection *>(body, "collection", [conn, &body](CBLCollection* collection) {
            with<CBLDocument *>(body, "document", [conn, collection] (CBLDocument* d) {
                CBLError* err = new CBLError();
                if(err->code!=0)
                    write_serialized_body(conn, err->code);
                else
                    write_serialized_body(conn, CBLCollection_DeleteDocument(collection, d, err));
            });
        });
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
                    string errMsg = to_string(CBLError_Message(&err));
                    throw domain_error(errMsg);
                }
            });
        });
        write_empty_body(conn);
    }

    //purge document in collection
    void collection_purgeDocument(json& body, mg_connection* conn) {
        with<CBLCollection *>(body, "collection", [conn, &body](CBLCollection* collection) {
            with<CBLDocument *>(body, "document", [conn, collection](CBLDocument* d) {
                CBLError* err = new CBLError();
                auto purge = CBLCollection_PurgeDocument(collection, d, err);
                if(err->code!=0)
                {
                    write_serialized_body(conn, err->code);
                }
                else
                    write_serialized_body(conn, purge);
            });
        });
    }

    //purge document by Id
    void collection_purgeDocumentID(json& body, mg_connection* conn) {
        const auto docID = body["docId"];
        with<CBLCollection *>(body, "collection", [conn, &docID](CBLCollection* collection) {
                CBLError* err = new CBLError();
                auto purge = CBLCollection_PurgeDocumentByID(collection, flstr(docID), err);
                if(err->code!=0)
                {
                    write_serialized_body(conn, err->code);
                }
                else
                    write_serialized_body(conn, purge);
            });
    }

    //Document expiration time and date
    void collection_getDocumentExpiration(json& body, mg_connection* conn) {
        with<CBLCollection *>(body, "collection", [conn,&body](CBLCollection* collection) {
            const auto docId = body["docId"];
            CBLError* err = new CBLError();
            const auto expirateDate = CBLCollection_GetDocumentExpiration(collection, flstr(docId), err);
            if(err->code!=0)
                write_serialized_body(conn, err->code);
            else {
                write_serialized_body(conn, CBLTimestamp(expirateDate));
            }
        });
    }

    //set document expiration
    void collection_setDocumentExpiration(json& body, mg_connection* conn) {
        with<CBLCollection *>(body, "collection", [conn, &body](CBLCollection* collection) {
            const auto docId = body["docId"];
            const auto expiration = body["expiration"];
            CBLError* err = new CBLError();
            const auto setExpiration = CBLCollection_SetDocumentExpiration(collection, flstr(docId), CBLTimestamp(expiration), err);
            if(err->code!=0)
                write_serialized_body(conn, err->code);
            else
                write_serialized_body(conn, setExpiration);
        });
    }

    //get mutuable document from the collection
    void collection_getMutableDocument(json& body, mg_connection* conn){
        with<CBLCollection *>(body, "collection", [conn, &body](CBLCollection* collection) {
            const auto docId = body["docId"];
            CBLError* err = new CBLError();
            const auto document = CBLCollection_GetMutableDocument(collection, flstr(docId), err);
            if(err->code!=0)
                write_serialized_body(conn, err->code);
            else
                write_serialized_body(conn, memory_map::store(document, CBLCollectionDocument_EntryDelete));
        });
    }

    //create value index
    void collection_createValueIndex(json& body, mg_connection* conn) {
        with<CBLCollection *>(body, "collection", [conn, &body](CBLCollection* collection) {
            CBLValueIndexConfiguration *config = new CBLValueIndexConfiguration();
            config->expressionLanguage = body["expressionLanguage"];
            config->expressions = flstr(body["expression"]);
            const auto name = body["name"];
            CBLError* err = new CBLError();
            bool createValueIndex = CBLCollection_CreateValueIndex(collection, flstr(name), *config , err);
            if(err->code!=0)
                write_serialized_body(conn, err->code);
            else
                write_serialized_body(conn, createValueIndex);
        });
    }

    //create full text index
    void collection_createFullTextIndex(json& body, mg_connection* conn) {
        with<CBLCollection *>(body, "collection", [conn,&body](CBLCollection* collection) {
            const auto name = flstr(body["name"]);
            CBLError *err = new CBLError();
            CBLFullTextIndexConfiguration *config = new CBLFullTextIndexConfiguration();
            config->expressionLanguage = body["expressionLanguage"];
            config->expressions = flstr(body["expressions"]);
            config->language = flstr(body["language"]);
            if(body["ignoreAccents"] == true)
                config->ignoreAccents = true;
            bool createFullTextIndex = CBLCollection_CreateFullTextIndex(collection, name, *config, err);
            if(err->code!=0)
                write_serialized_body(conn, err->code);
            else
                write_serialized_body(conn, createFullTextIndex);
        });
    }

    //delete index
    void collection_deleteIndex(json& body, mg_connection* conn) {
        with<CBLCollection *>(body, "collection", [conn, &body](CBLCollection* collection) {
            auto name = body["name"];
            CBLError* err = new CBLError();
            auto deleteIndex = CBLCollection_DeleteIndex(collection, flstr(name), err);
            if(err->code!=0)
                write_serialized_body(conn, err->code);
            else
                write_serialized_body(conn, deleteIndex);
        });
    }

    //get index names
    void collection_getIndexNames(json& body, mg_connection* conn) {
        with<CBLCollection *>(body, "collection", [conn, &body](CBLCollection* collection) {
            json keys(0, nullptr);
            CBLError* err = new CBLError();
            FLMutableArray index_names = CBLCollection_GetIndexNames(collection, err);
            if(err->code!=0)
                write_serialized_body(conn, err->code);
            else {
                FLArrayIterator i;
                FLArrayIterator_Begin(index_names, &i);
                do{
                    const auto indexName = FLArrayIterator_GetValue(&i);
                    FLStringResult jsonString = FLValue_ToString(indexName);
                    keys.push_back(jsonString);
                } while(FLArrayIterator_Next(&i));
                write_serialized_body(conn, keys);
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
            const auto docId = body["docId"];
            CBLListenerToken* token = CBLCollection_AddDocumentChangeListener(collection, flstr(docId), CBLCollection_DummyDocumentListener, nullptr);
            write_serialized_body(conn, memory_map::store(token, nullptr));
        });
    }
}





