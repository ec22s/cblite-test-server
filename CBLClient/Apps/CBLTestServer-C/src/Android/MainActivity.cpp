#include <android_native_app_glue.h>
#include <jni.h>
#include <android/log.h>
#include <civetweb.h>
#include <string>
#include <sys/stat.h>

#include "../TestServer.h"
#include "../FilePathResolver.h"

android_app* GlobalApp;

extern "C" {
    void handle_cmd(android_app* pApp, int32_t cmd) {

    }

    void android_main(struct android_app* pApp) {
        __android_log_print(ANDROID_LOG_INFO, "TestServer", "Entering android_main...");

        std::string foo = file_resolution::resolve_path("PrebuiltDB.cblite2.zip", true);
        __android_log_print(ANDROID_LOG_INFO, "TestServer", "%s", foo.c_str());

        GlobalApp = pApp;
        pApp->onAppCmd = handle_cmd;

        mg_init_library(0);
        TestServer server{};
        server.start();
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