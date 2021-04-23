#include "DatabaseConfigurationMethods.h"
#include "MemoryMap.h"
#include "Router.h"
#include "Defines.h"

#include <cbl/CouchbaseLite.h>
#include <string>
using namespace std;
using namespace nlohmann;

static void CBLDatabaseConfiguration_EntryDelete(void* ptr) {
    // Sigh...
    auto* config = static_cast<CBLDatabaseConfiguration *>(ptr);
    free(const_cast<void *>(config->directory.buf));
    free(config);
}

namespace database_configuration_methods {
    void database_configuration_configure(json& body, mg_connection* conn) {
        auto* databaseConfig = static_cast<CBLDatabaseConfiguration *>(calloc(1, sizeof(CBLDatabaseConfiguration)));
        if(body.contains("directory")) {
            const auto directory = body["directory"].get<string>();
            char* allocated = new char[directory.size()];
            memcpy(allocated, directory.c_str(), directory.size());
            databaseConfig->directory = { allocated, directory.size() };
        }
    }
}