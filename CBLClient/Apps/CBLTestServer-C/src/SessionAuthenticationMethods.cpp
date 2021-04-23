#include "SessionAuthenticationMethods.h"
#include "MemoryMap.h"
#include "Router.h"
#include "Defines.h"

#include <cbl/CouchbaseLite.h>
#include <string>
using namespace std;
using namespace nlohmann;

static void CBLAuthenticator_EntryDelete(void* ptr) {
    CBLAuth_Free(static_cast<CBLAuthenticator*>(ptr));
}

namespace session_authentication_methods {
    void session_authentication_create(json& body, mg_connection* conn) {
        const auto sessionId = body["sessionId"].get<string>();
        const auto cookieName = body["cookieName"].get<string>();
        auto* authentication = CBLAuth_NewSession(flstr(sessionId), flstr(cookieName));
        const auto id = memory_map::store(authentication, CBLAuthenticator_EntryDelete);
        write_serialized_body(conn, id);
    }
}