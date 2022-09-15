#include "ScopeMethods.hpp"
#include "MemoryMap.h"
#include "Router.h"
#include "FleeceHelpers.h"
#include "Defines.h"

#include INCLUDE_CBL(CouchbaseLite.h)
using namespace std;
using namespace nlohmann;

void CBLScope_EntryDelete (void* ptr)
{
    CBLScope_Release(static_cast<CBLScope*>(ptr));
}

void CBLScopeCollection_EntryDelete (void* ptr)
{
    CBLCollection_Release(static_cast<CBLCollection*>(ptr));
}

namespace scope_methods {
    //default scope object
    void scope_defaultScope(json& body, mg_connection* conn){
        with<CBLDatabase *>(body, "database", [conn](CBLDatabase* db) {
            CBLError err = {};
            CBLScope* scope = CBLDatabase_DefaultScope(db, &err);
            if(err.code!=0)
                write_serialized_body(conn, CBLError_Message(&err));
            else {
                write_serialized_body(conn, memory_map::store(scope, CBLScope_EntryDelete));
            }
        });
    }

    //return scope name using scope object
    void scope_scopeName(json &body, mg_connection* conn) {
        with<CBLScope *>(body, "scope", [conn](CBLScope* scope) {
            write_serialized_body(conn, CBLScope_Name(scope));
        });
    }

    //returns name of all collection in the scope
    void scope_collectionNames (json& body, mg_connection* conn) {
        with<CBLScope *>(body, "scope",[conn](CBLScope* scope) {
            CBLError err = {};
            FLMutableArray collectionNames = CBLScope_CollectionNames(scope, &err);
            if(err.code!=0)
                write_serialized_body(conn, CBLError_Message(&err));
            else {
                write_serialized_body(conn, reinterpret_cast<const FLValue>(collectionNames));
            }
        });
    }

    //return exisitng collection in the scope with the given name
    void scope_collection (json& body, mg_connection* conn) {
        with<CBLScope *>(body, "scope", [conn,&body](CBLScope* scope) {
            const auto collectionName = flstr(body["collectionName"]);
            CBLError err = {};
            CBLCollection *collection = CBLScope_Collection(scope, collectionName, &err);
            if(err.code!=0)
                write_serialized_body(conn, CBLError_Message(&err));
            else
                write_serialized_body(conn, memory_map::store(collection,CBLScopeCollection_EntryDelete));
        });
    }
}
