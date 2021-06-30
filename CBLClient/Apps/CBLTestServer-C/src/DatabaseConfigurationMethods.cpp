#include "DatabaseConfigurationMethods.h"
#include "MemoryMap.h"
#include "Router.h"
#include "Defines.h"

#include INCLUDE_CBL(CouchbaseLite.h)
#include <string>
using namespace std;
using namespace nlohmann;

#ifdef __ANDROID__
#include <android_native_app_glue.h>
#include <sys/stat.h>
extern android_app* GlobalApp;
#endif

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
#ifdef __ANDROID__
        else {
            // The default directory provided by C is not always writable
            string internalData = GlobalApp->activity->internalDataPath;
            if(internalData[internalData.size() - 1] != '/') {
                internalData += "/";
            }

            internalData += "db/";
            mkdir(internalData.c_str(), 0755);
            char* allocated = new char[internalData.size()];
            memcpy(allocated, internalData.c_str(), internalData.size());
            databaseConfig->directory = { allocated, internalData.size() };
        }
#endif

        write_serialized_body(conn, memory_map::store(databaseConfig, CBLDatabaseConfiguration_EntryDelete));
    }

    void database_configuration_setEncryptionKey(json &body, mg_connection *conn) {
        with<CBLDatabaseConfiguration *>(body, "config", [body, conn](CBLDatabaseConfiguration* dbconfig) {
            auto password = body["password"].get<string>();
            CBLEncryptionKey encryptionKey;
            if(!CBLEncryptionKey_FromPassword(&encryptionKey, flstr(password))) {
                mg_send_http_error(conn, 500, "Error creating encryption key");
                return;
            }
            
            dbconfig->encryptionKey = encryptionKey;
            mg_send_http_ok(conn, "application/text", 0);
        });
    }
}
