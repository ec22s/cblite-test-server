#include <android_native_app_glue.h>
#include <jni.h>
#include <android/log.h>
#include <civetweb.h>
#include <string>
#include <sys/stat.h>

#include "../TestServer.h"
#include "../FilePathResolver.h"
#include <cbl/CouchbaseLite.h>
using namespace std;

android_app* GlobalApp;

extern "C" {
    void handle_cmd(android_app* pApp, int32_t cmd) {

    }

    string filesDir;
    string tmpDir;

    void android_main(struct android_app* pApp) {
        filesDir = string(pApp->activity->internalDataPath) + "/.couchbase";
        tmpDir = string(pApp->activity->externalDataPath) + "/CouchbaseLiteTemp";
        mkdir(filesDir.c_str(), 0755);
        mkdir(tmpDir.c_str(), 0755);
        CBLInitContext init {
            .filesDir = filesDir.c_str(),
            .tempDir = tmpDir.c_str()
        };

        CBLError err;
        if(!CBL_Init(init, &err)) {
            __android_log_print(ANDROID_LOG_FATAL, "TestServer", "Failed to init CBL (%d / %d)", err.domain, err.code);
            return;
        }

        __android_log_print(ANDROID_LOG_INFO, "TestServer", "Entering android_main...");
        GlobalApp = pApp;

        pApp->onAppCmd = handle_cmd;

        mg_init_library(0);
        TestServer server{};
        server.start();
    #ifdef COUCHBASE_ENTERPRISE
         __android_log_print(ANDROID_LOG_INFO, "TestServer", "Using CBL C version %s-%d (Enterprise)", CBLITE_VERSION, CBLITE_BUILD_NUMBER);
    #else
         __android_log_print(ANDROID_LOG_INFO, "TestServer", "Using CBL C version %s-%d", CBLITE_VERSION, CBLITE_BUILD_NUMBER);
    #endif
        __android_log_print(ANDROID_LOG_INFO, "TestServer", "Listening on port %d...", TestServer::PORT);

        int events;
        android_poll_source *pSource;
        do {
            if (ALooper_pollAll(0, nullptr, &events, (void **) &pSource) >= 0) {
                if (pSource) {
                    pSource->process(pApp, pSource);
                }
            }
        } while (!pApp->destroyRequested);
    }
}