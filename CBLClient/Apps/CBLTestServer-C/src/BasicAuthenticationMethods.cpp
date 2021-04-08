#include "BasicAuthenticationMethods.h"
#include "MemoryMap.h"
#include "Router.h"

#include <cbl/CBLReplicator.h>
#include <string>
using namespace std;

static void CBLAuthenticator_EntryDelete(void* ptr) {
    CBLAuth_Free(static_cast<CBLAuthenticator*>(ptr));
}

namespace basic_authentication_methods {
    void basic_authentication_create(nlohmann::json& body, mg_connection* conn) {
        const auto username = body["username"].get<string>();
        const auto password = body["password"].get<string>();
        auto* authentication = CBLAuth_NewPassword({ username.data(), username.size() }, { password.data(), password.size() });
        const auto id = memory_map::store(authentication, CBLAuthenticator_EntryDelete);
        write_serialized_body(conn, id);
    }

}