#include "DatabaseMethods.h"
#include "MemoryMap.h"
#include "Router.h"
#include "Defer.hh"
#include "FleeceHelpers.h"
#include "Defines.h"
#include "FilePathResolver.h"
extern "C" {
    #include "cdecode.h"
}

#include INCLUDE_CBL(CouchbaseLite.h)
using namespace std;
using namespace nlohmann;

static void CBLDatabase_EntryDelete(void* ptr) {
    CBLDatabase_Release(static_cast<CBLDatabase *>(ptr));
}
void CBLDatabaseScope_EntryDelete (void* ptr)
{
    CBLScope_Release(static_cast<CBLScope*>(ptr));
}

void CBLDocument_EntryDelete(void* ptr);

namespace database_methods {
    void database_create(json& body, mg_connection* conn) {
        const auto name = body["name"].get<string>();
        CBLDatabaseConfiguration* config = nullptr;
        if(body.contains("config")) {
            config = static_cast<CBLDatabaseConfiguration*>(memory_map::get(body["config"].get<string>()));
        }

        CBLError err;
        CBLDatabase* db;
        TRY((db = CBLDatabase_Open({name.data(), name.size()}, config, &err)), err)
        write_serialized_body(conn, memory_map::store(db, CBLDatabase_EntryDelete));
    }

    void database_compact(json& body, mg_connection* conn) {
        with<CBLDatabase *>(body, "database", [](CBLDatabase* db)
        {
            CBLError err;
            TRY(CBLDatabase_PerformMaintenance(db, kCBLMaintenanceTypeCompact, &err), err)
        });

        write_empty_body(conn);
    }

    void database_close(json& body, mg_connection* conn) {
        with<CBLDatabase *>(body, "database", [](CBLDatabase* db)
        {
            CBLError err;
            TRY(CBLDatabase_Close(db, &err), err)
        });

        write_empty_body(conn);
    }

    void database_getPath(json& body, mg_connection* conn) {
        with<CBLDatabase *>(body, "database", [conn](CBLDatabase* db)
        {
            string path = to_string(CBLDatabase_Path(db));
            write_serialized_body(conn, path);
        });
    }

    void database_deleteDB(json& body, mg_connection* conn) {
        with<CBLDatabase *>(body, "database", [](CBLDatabase* db)
        {
            CBLError err;
            TRY(CBLDatabase_Delete(db, &err), err)
        });

        write_empty_body(conn);
    }

    void database_delete(json& body, mg_connection* conn) {
        with<CBLDatabase *>(body, "database", [&body, conn](CBLDatabase* db)
        {
            with<CBLDocument *>(body, "document", [db, conn](CBLDocument* doc)
            {
                CBLError err;
                TRY(CBLDatabase_DeleteDocument(db, doc, &err), err)
            });
        });

        write_empty_body(conn);
    }

    void database_deleteBulkDocs(json& body, mg_connection* conn) {
        const auto doc_ids = body["doc_ids"];
        with<CBLDatabase *>(body, "database", [&doc_ids](CBLDatabase* db)
        {
            CBLError err;
            TRY(CBLDatabase_BeginTransaction(db, &err), err)

            bool success = false;
            DEFER {
                TRY(CBLDatabase_EndTransaction(db, success, &err), err)
            };

            for(const auto& val : doc_ids) {
                const auto docID = val.get<string>();
                const CBLDocument* doc = CBLDatabase_GetDocument(db, flstr(docID), &err);
                DEFER {
                    CBLDocument_Release(doc);
                };

                if(!doc) {
                    if(err.code == 0) {
                        continue;
                    }

                    string msgStr = to_string(CBLError_Message(&err));
                    throw domain_error(msgStr);
                }

                TRY(CBLDatabase_DeleteDocument(db, doc, &err), err)
            }

            success = true;
        });

        write_empty_body(conn);
    }

    void database_getName(json& body, mg_connection* conn) {
        with<CBLDatabase *>(body, "database", [conn](CBLDatabase* db)
        {
            const auto dbName = CBLDatabase_Name(db);
            write_serialized_body(conn, to_string(dbName));
        });
    }

    void database_getDocument(json& body, mg_connection* conn) {
        const auto docId = body["id"].get<string>();
        const CBLDocument* doc;
        with<CBLDatabase *>(body, "database", [&docId, &doc](CBLDatabase* db)
        {
            CBLError err;
            doc = CBLDatabase_GetDocument(db, flstr(docId), &err);
            if(!doc && err.code != 0) {
                std::string errMsg = to_string(CBLError_Message(&err));
                throw std::domain_error(errMsg);
            }
        });

        if(!doc) {
            write_empty_body(conn);
            return;
        }

        write_serialized_body(conn, memory_map::store(doc, CBLDocument_EntryDelete));
    }

    void database_saveDocuments(json& body, mg_connection* conn) {
        const auto docDict = body["documents"];
        with<CBLDatabase *>(body, "database", [&docDict](CBLDatabase* db)
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
                    } else {
                        writeFleece(body, bodyKey, bodyValue);
                    }
                }

                CBLDocument_SetProperties(doc, body);
                FLMutableDict_Release(body);
                TRY(CBLDatabase_SaveDocument(db, doc, &err), err)
            }

            success = true;
        });

        write_empty_body(conn);
    }

    void database_purge(json& body, mg_connection* conn) {
        with<CBLDatabase *>(body, "database", [conn, &body](CBLDatabase* db)
        {
            with<CBLDocument *>(body, "document", [conn, db](CBLDocument* d)
            {
                CBLError err;
                TRY(CBLDatabase_PurgeDocument(db, d, &err), err)
            });
        });

        write_empty_body(conn);
    }

    void database_save(json& body, mg_connection* conn) {
        with<CBLDatabase *>(body, "database", [conn, &body](CBLDatabase* db)
        {
            with<CBLDocument *>(body, "document", [conn, db](CBLDocument* d)
            {
                CBLError err;
                TRY(CBLDatabase_SaveDocument(db, d, &err), err)
            });
        });

        write_empty_body(conn);
    }

    void database_saveWithConcurrency(json& body, mg_connection* conn) {
        string concurrencyControlType;
        if(body.contains("concurrencyControlType")) {
            concurrencyControlType = body["concurrencyControlType"].get<string>();
        }

        with<CBLDatabase *>(body, "database", [conn, &body, &concurrencyControlType](CBLDatabase* db)
        {
            with<CBLDocument *>(body, "document", [conn, db, &concurrencyControlType](CBLDocument* d)
            {
                auto concurrencyType = kCBLConcurrencyControlLastWriteWins;
                if(concurrencyControlType == "failOnConflict") {
                    concurrencyType = kCBLConcurrencyControlFailOnConflict;
                }

                CBLError err {};
                if(!CBLDatabase_SaveDocumentWithConcurrencyControl(db, d, concurrencyType, &err) && err.code != (int)kCBLErrorConflict) {
                    string errMsg = to_string(CBLError_Message(&err));
                    throw domain_error(errMsg);
                }
            });
        });

        write_empty_body(conn);
    }

    void database_deleteWithConcurrency(json& body, mg_connection* conn) {
        string concurrencyControlType;
        if(body.contains("concurrencyControlType")) {
            concurrencyControlType = body["concurrencyControlType"].get<string>();
        }

        with<CBLDatabase *>(body, "database", [conn, &body, &concurrencyControlType](CBLDatabase* db)
        {
            with<CBLDocument *>(body, "document", [conn, db, &concurrencyControlType](CBLDocument* d)
            {
                auto concurrencyType = kCBLConcurrencyControlLastWriteWins;
                if(concurrencyControlType == "failOnConflict") {
                    concurrencyType = kCBLConcurrencyControlFailOnConflict;
                }

                CBLError err {};
                if(!CBLDatabase_DeleteDocumentWithConcurrencyControl(db, d, concurrencyType, &err) && err.code != (int)kCBLErrorConflict) {
                    string errMsg = to_string(CBLError_Message(&err));
                    throw domain_error(errMsg);
                }
            });
        });

        write_empty_body(conn);
    }

    void database_getCount(json& body, mg_connection* conn) {
        with<CBLDatabase *>(body, "database", [conn](CBLDatabase* db)
        {
            write_serialized_body(conn, CBLDatabase_Count(db));
        });
    }

    void database_getDocIds(json& body, mg_connection* conn) {
        const auto limit = body["limit"].get<int>();
        const auto offset = body["offset"].get<int>();
        const auto queryString = "select meta().id from _ limit " + to_string(limit) + " offset " + to_string(offset);
        with<CBLDatabase *>(body, "database", [conn, &queryString](CBLDatabase* db)
        {
            CBLError err;
            CBLQuery* query;
            TRY((query = CBLDatabase_CreateQuery(db, kCBLN1QLLanguage, flstr(queryString), nullptr, &err)), err)
            DEFER {
                CBLQuery_Release(query);
            };

            CBLResultSet* results;
            TRY((results = CBLQuery_Execute(query, &err)), err)
            json ids = json::array();
            while(CBLResultSet_Next(results)) {
                FLString nextId = FLValue_AsString(CBLResultSet_ValueAtIndex(results, 0));
                ids.push_back(string(static_cast<const char *>(nextId.buf), nextId.size));
            }

            CBLResultSet_Release(results);
            write_serialized_body(conn, ids);
        });
    }

    void database_getDocuments(json& body, mg_connection* conn) {
        const auto ids = body["ids"];
        FLMutableDict retVal = FLMutableDict_New();

        vector<const CBLDocument *> docs;
        DEFER {
            for(const auto* doc : docs) {
                CBLDocument_Release(doc);
            }

            FLMutableDict_Release(retVal);
        };
        
        with<CBLDatabase *>(body, "database", [retVal, &ids, &docs](CBLDatabase* db)
        {
            for(const auto& idJson : ids) {
                const auto id = idJson.get<string>();
                const CBLDocument* doc;
                CBLError err;
                TRY((doc = CBLDatabase_GetDocument(db, flstr(id), &err)), err);
                if(!doc) {
                    continue;
                }

                auto* slot = FLMutableDict_Set(retVal, flstr(id));
                FLSlot_SetValue(slot, reinterpret_cast<FLValue>(CBLDocument_Properties(doc)));
            }
        });

        write_serialized_body(conn, reinterpret_cast<FLValue>(retVal));
    }

    void database_updateDocument(json& body, mg_connection* conn) {
        const auto id = body["id"].get<string>();
        const auto data = body["data"];
        with<CBLDatabase *>(body, "database", [&id, &data](CBLDatabase* db)
        {
            CBLDocument* doc;
            CBLError err;
            TRY(doc = CBLDatabase_GetMutableDocument(db, flstr(id), &err), err)
            DEFER {
                CBLDocument_Release(doc);
            };

            FLMutableDict newContent = FLMutableDict_New();
            for(auto& [key, value] : data.items()) {
                writeFleece(newContent, key, value);
            }

            CBLDocument_SetProperties(doc, newContent);
            FLMutableDict_Release(newContent);
            TRY(CBLDatabase_SaveDocument(db, doc, &err), err)
        });

        write_empty_body(conn);
    }

    void database_updateDocuments(json& body, mg_connection* conn) {
        const auto docDict = body["documents"];
        with<CBLDatabase *>(body, "database", [&docDict](CBLDatabase* db)
        {
            CBLError err;
            TRY(CBLDatabase_BeginTransaction(db, &err), err)
            auto success = false;
            DEFER {
                TRY(CBLDatabase_EndTransaction(db, success, &err), err)
            };

            for(auto& [key, value] : docDict.items()) {
                CBLDocument* doc;
                TRY((doc = CBLDatabase_GetMutableDocument(db,flstr(key), &err)), err)
                DEFER {
                    CBLDocument_Release(doc);
                };

                FLMutableDict newContent = FLMutableDict_New();
                for(auto& [key, value] : value.items()) {
                    writeFleece(newContent, key, value);
                }

                CBLDocument_SetProperties(doc, newContent);
                FLMutableDict_Release(newContent);
                TRY(CBLDatabase_SaveDocument(db, doc, &err), err)
            }

            success = true;
        });

        write_empty_body(conn);
    }

    void database_exists(json& body, mg_connection* conn) {
        const auto name = body["name"].get<string>();
        const auto directory = body["directory"].get<string>();
        write_serialized_body(conn, CBL_DatabaseExists(flstr(name), flstr(directory)));
    }

    void database_copy(json& body, mg_connection* conn) {
        const auto dbName = body["dbName"].get<string>();
        const auto dbPath = body["dbPath"].get<string>();
        char cwd[1024];
        cbl_getcwd(cwd, 1024);
        const auto databasePath = string(cwd) + DIRECTORY_SEPARATOR + dbPath;
        const auto* dbConfig = static_cast<const CBLDatabaseConfiguration *>(memory_map::get(body["dbConfig"].get<string>()));
        CBLError err;
        TRY(CBL_CopyDatabase(flstr(databasePath), flstr(dbName), dbConfig, &err), err)
        write_empty_body(conn);
    }

    void database_getPreBuiltDb(json& body, mg_connection* conn) {
        string dbPath = file_resolution::resolve_path(body["dbPath"].get<string>(), true);
        dbPath += "/";
        write_serialized_body(conn, dbPath);
    }

    void database_changeEncryptionKey(json& body, mg_connection* conn) {
#ifndef COUCHBASE_ENTERPRISE
        mg_send_http_error(conn, 501, "Not supported in CE edition");
#else
        with<CBLDatabase *>(body, "database", [body, conn](CBLDatabase* db) {
            CBLError err;
            auto password = body["password"].get<string>();
            if(password == "nil") {
                TRY(CBLDatabase_ChangeEncryptionKey(db, nullptr, &err), err);
            } else {
                CBLEncryptionKey encryptionKey;
                if(!CBLEncryptionKey_FromPassword(&encryptionKey, flstr(password))) {
                    mg_send_http_error(conn, 500, "Failed to create encryption key");
                    return;
                }
                
                TRY(CBLDatabase_ChangeEncryptionKey(db, &encryptionKey, &err), err);
            }

            write_empty_body(conn);
        });
#endif
    }

//names of all scopes in the database
void database_allScope(json& body, mg_connection* conn) {
    with<CBLDatabase *>(body,"database",[conn](CBLDatabase* db){
        CBLError err = {};
        auto scopeNames = CBLDatabase_ScopeNames(db, &err);
        if(err.code!=0)
            write_serialized_body(conn, CBLError_Message(&err));
        else {
            write_serialized_body(conn, reinterpret_cast<const FLValue>(scopeNames));
        }
    });
}

//returns existing scope object with the given name
void database_scope(json& body, mg_connection* conn) {
    auto scopeName = body["scopeName"].get<string>();
    with<CBLDatabase *>(body,"database",[conn,&scopeName](CBLDatabase* db){
        CBLError err = {};
        auto scope = CBLDatabase_Scope(db, flstr(scopeName), &err);
        if(err.code!=0)
            write_serialized_body(conn, CBLError_Message(&err));
        else if(!scope)
            write_serialized_body(conn, NULL);
        else
            write_serialized_body(conn, memory_map::store(scope, CBLDatabaseScope_EntryDelete));
    });
}
}
