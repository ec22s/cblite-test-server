#include "DatabaseMethods.h"
#include "MemoryMap.h"
#include "Router.h"
#include "ValueSerializer.h"
#include "Defer.hh"

#include <cbl/CBLDatabase.h>
#include <cbl/CBLDocument.h>
using namespace std;
using namespace nlohmann;

static void CBLDatabase_DeleteEntry(void* ptr) {
    CBLDatabase_Release(static_cast<CBLDatabase *>(ptr));
}

namespace database_methods {
    void database_create(json& body, mg_connection* conn) {
        const auto name = body["name"].get<string>();
        CBLDatabaseConfiguration* config = nullptr;
        if(body.contains("config")) {
            config = static_cast<CBLDatabaseConfiguration*>(memory_map::get(body["config"].get<string>()));
        }

        CBLError err;
        CBLDatabase* db = CBLDatabase_Open({name.data(), name.size()}, config, &err);
        if(!db) {
            string msgStr = from_slice_result(CBLError_Message(&err));
            throw domain_error(msgStr);
        }

        write_serialized_body(conn, memory_map::store(db, CBLDatabase_DeleteEntry));
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
            string path = from_slice_result(CBLDatabase_Path(db));
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
        with<CBLDatabase *>(body, "database", [&body, conn](CBLDatabase* db)
        {
            CBLError err;
            TRY(CBLDatabase_BeginTransaction(db, &err), err)

            bool success = false;
            DEFER {
                TRY(CBLDatabase_EndTransaction(db, success, &err), err)
            };

            for(const auto& val : body) {
                const auto docID = val.get<string>();
                const CBLDocument* doc = CBLDatabase_GetDocument(db, {docID.data(), docID.size()}, &err);
                if(!doc) {
                    if(err.domain == CBLDomain && err.code == CBLErrorNotFound) {
                        continue;
                    }

                    string msgStr = from_slice_result(CBLError_Message(&err));
                    throw domain_error(msgStr);
                }

                TRY(CBLDatabase_DeleteDocument(db, doc, &err), err)
            }

            success = true;
        });

        mg_send_http_ok(conn, "text/plain", 0);
    }


}