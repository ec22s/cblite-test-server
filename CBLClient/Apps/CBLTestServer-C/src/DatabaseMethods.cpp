#include "DatabaseMethods.h"
#include "MemoryMap.h"
#include "Router.h"
#include "Defer.hh"
#include "FleeceHelpers.h"
#include "Defines.h"

#include <cbl/CouchbaseLite.h>
using namespace std;
using namespace nlohmann;

static void CBLDatabase_EntryDelete(void* ptr) {
    CBLDatabase_Release(static_cast<CBLDatabase *>(ptr));
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

        mg_send_http_ok(conn, "text/plain", 0);
    }

    void database_close(json& body, mg_connection* conn) {
        with<CBLDatabase *>(body, "database", [](CBLDatabase* db)
        {
            CBLError err;
            TRY(CBLDatabase_Close(db, &err), err)
        });

        mg_send_http_ok(conn, "text/plain", 0);
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

        mg_send_http_ok(conn, "text/plain", 0);
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

        mg_send_http_ok(conn, "text/plain", 0);
    }

    void database_deleteBulkDocs(json& body, mg_connection* conn) {
        with<CBLDatabase *>(body, "database", [&body](CBLDatabase* db)
        {
            CBLError err;
            TRY(CBLDatabase_BeginTransaction(db, &err), err)

            bool success = false;
            DEFER {
                TRY(CBLDatabase_EndTransaction(db, success, &err), err)
            };

            for(const auto& val : body) {
                const auto docID = val.get<string>();
                const CBLDocument* doc = CBLDatabase_GetDocument(db, flstr(docID), &err);
                DEFER {
                    CBLDocument_Release(doc);
                };

                if(!doc) {
                    if(err.domain == CBLDomain && err.code == CBLErrorNotFound) {
                        continue;
                    }

                    string msgStr = to_string(CBLError_Message(&err));
                    throw domain_error(msgStr);
                }

                TRY(CBLDatabase_DeleteDocument(db, doc, &err), err)
            }

            success = true;
        });

        mg_send_http_ok(conn, "text/plain", 0);
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
            DEFER {
                CBLDocument_Release(doc);
            };

            if(!doc && (err.domain != CBLDomain || err.code != CBLErrorNotFound)) {
                std::string errMsg = to_string(CBLError_Message(&err));
                throw std::domain_error(errMsg);
            }
        });

        if(!doc) {
            mg_send_http_ok(conn, "application/text", 0);
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
                    writeFleece(body, bodyKey, bodyValue);
                }

                CBLDocument_SetProperties(doc, body);
                FLMutableDict_Release(body);
                TRY(CBLDatabase_SaveDocument(db, doc, &err), err)
            }

            success = true;
        });

        mg_send_http_ok(conn, "application/text", 0);
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

        mg_send_http_ok(conn, "application/text", 0);
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

                CBLError err;
                TRY(CBLDatabase_SaveDocumentWithConcurrencyControl(db, d, concurrencyType, &err), err)
            });
        });

        mg_send_http_ok(conn, "application/text", 0);
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

                CBLError err;
                TRY(CBLDatabase_DeleteDocumentWithConcurrencyControl(db, d, concurrencyType, &err), err)
            });
        });

        mg_send_http_ok(conn, "application/text", 0);
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
        const auto queryString = "select meta().id from db limit " + to_string(limit) + " offset " + to_string(offset);
        with<CBLDatabase *>(body, "database", [conn, &queryString](CBLDatabase* db)
        {
            CBLError err;
            CBLQuery* query;
            TRY((query = CBLQuery_New(db, kCBLN1QLLanguage, flstr(queryString), nullptr, &err)), err)
            DEFER {
                CBLQuery_Release(query);
            };

            CBLResultSet* results;
            TRY((results = CBLQuery_Execute(query, &err)), err)
            json ids(0, nullptr);
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
        vector<const CBLDocument *> docs;
        DEFER {
            for(const auto* doc : docs) {
                CBLDocument_Release(doc);
            }
        };

        with<CBLDatabase *>(body, "database", [&ids, &docs](CBLDatabase* db)
        {
            for(const auto& idJson : ids) {
                const auto id = idJson.get<string>();
                const CBLDocument* doc;
                CBLError err;
                TRY((doc = CBLDatabase_GetDocument(db, flstr(id), &err)), err)
                docs.push_back(doc);
            }
        });

        if(docs.size() != ids.size()) {
            mg_send_http_error(conn, 500, "Didn't successfully get all docs");
            return;
        }

        FLMutableDict retVal = FLMutableDict_New();
        for(uint32_t i = 0; i < ids.size(); i++) {
            const auto id = ids[i].get<string>();
            const auto* doc = docs[i];
            auto* slot = FLMutableDict_Set(retVal, flstr(id));
            FLSlot_SetValue(slot, reinterpret_cast<FLValue>(CBLDocument_Properties(doc)));
        }

        write_serialized_body(conn, retVal);
        FLMutableDict_Release(retVal);
    }

    void database_updateDocument(json& body, mg_connection* conn) {
        const auto id = body["id"].get<string>();
        const auto data = body["data"];
        with<CBLDatabase *>(body, "database", [&id, &data](CBLDatabase* db)
        {
            CBLDocument* doc;
            CBLError err;
            TRY(CBLDatabase_GetMutableDocument(db, flstr(id), &err), err)
            DEFER {
                CBLDocument_Release(doc);
            };

            FLMutableDict newContent = FLMutableDict_New();
            for(auto& [key, value] : data.items()) {
                writeFleece(newContent, key, value);
            }

            TRY(CBLDatabase_SaveDocument(db, doc, &err), err)
        });

        mg_send_http_ok(conn, "application/text", 0);
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

                TRY(CBLDatabase_SaveDocument(db, doc, &err), err)
            }

            success = true;
        });

        mg_send_http_ok(conn, "application/text", 0);
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
        const auto databasePath = string(cwd) + DIRECTORY_SEPARATOR + dbName;
        const auto* dbConfig = static_cast<const CBLDatabaseConfiguration *>(memory_map::get(body["dbConfig"].get<string>()));
        CBLError err;
        TRY(CBL_CopyDatabase(flstr(databasePath), flstr(dbName), dbConfig, &err), err)
        mg_send_http_ok(conn, "application/text", 0);
    }

    void database_getPreBuiltDb(json& body, mg_connection* conn) {
        // TODO: Need per platform implementation
        throw std::domain_error("Not implemented");
    }
}