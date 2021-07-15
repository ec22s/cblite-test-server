#include "QueryMethods.h"
#include "MemoryMap.h"
#include "Router.h"
#include "Defines.h"

using namespace nlohmann;
using namespace std;
using namespace fleece;

#include INCLUDE_CBL(CouchbaseLite.h)

static void CBLQuery_EntryDelete(void* ptr) {
    CBLQuery_Release(static_cast<CBLQuery *>(ptr));
}

static void CBLQuery_DummyListener(void* context, CBLQuery* q, CBLListenerToken* token) {

}

namespace query_methods {
    void query_selectAll(json& body, mg_connection* conn) {
        with<CBLDatabase *>(body, "database", [conn](CBLDatabase* db) {
            CBLQuery* query;
            CBLError err;
            TRY(query = CBLDatabase_CreateQuery(db, kCBLN1QLLanguage, FLSTR("select * from _"), nullptr, &err), err);
            write_serialized_body(conn, memory_map::store(query, CBLQuery_EntryDelete));
        });
    }

    void query_addChangeListener(json& body, mg_connection* conn) {
        with<CBLQuery *>(body, "query", [conn](CBLQuery* q) {
            CBLListenerToken* token = CBLQuery_AddChangeListener(q, CBLQuery_DummyListener, nullptr);
            write_serialized_body(conn, memory_map::store(token, nullptr));
        });
    }

    void query_removeChangeListener(json& body, mg_connection* conn) {
        with<CBLListenerToken *>(body, "changeListener", [conn](CBLListenerToken* token) {
            CBLListener_Remove(token);
        });

        mg_send_http_ok(conn, "application/text", 0);
    }
}