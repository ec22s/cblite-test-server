#include "QueryMethods.h"
#include "MemoryMap.h"
#include "Router.h"
#include "Defines.h"
#include "Defer.hh"

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

    void query_docsLimitOffset(json& body, mg_connection* conn) {
        auto limit = body["limit"].get<int64_t>();
        auto offset = body["offset"].get<int64_t>();
        with<CBLDatabase *>(body, "database", [conn, limit, offset](CBLDatabase* db) {
            CBLQuery* query;
            CBLError err;
            stringstream ss;
            json retVal = json::array();
            ss << "SELECT * FROM _ LIMIT " << limit << " OFFSET " << offset;
            TRY(query = CBLDatabase_CreateQuery(db, kCBLN1QLLanguage, flstr(ss.str()), nullptr, &err), err);
            DEFER {
                CBLQuery_Release(query);
            };

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

            write_serialized_body(conn, retVal);
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

        write_empty_body(conn);
    }
}