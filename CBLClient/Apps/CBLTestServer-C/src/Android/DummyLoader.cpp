#include <android/native_activity.h>
#include <android/log.h>
#include <dlfcn.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#define LIB_PATH "/data/data/com.couchbase.testsuite/lib/"

void* load_lib(const char* path) {
    static char buffer[PATH_MAX];
    void* handle = dlopen(path, RTLD_NOW);
    if(!handle) {
        sprintf(buffer, "%s%s", LIB_PATH, path);
        handle = dlopen(buffer, RTLD_NOW);
        if(!handle) {
            __android_log_print(ANDROID_LOG_ERROR, "dummyloader", "dl open '%s' failed (%s)",
                                path, strerror(errno));
            exit(1);
        }
    }

    return handle;
}

void ANativeActivity_onCreate(ANativeActivity * app, void * ud, size_t udsize)
{
    __android_log_print(ANDROID_LOG_INFO, "dummyloader", "Loaded bootstrap");
    load_lib("libcblite.so");
    __android_log_print(ANDROID_LOG_INFO, "dummyloader", "Loaded CBL-C");
    auto* main = (void (*)(ANativeActivity*, void*, size_t))dlsym(load_lib( "libtestserver.so"), "ANativeActivity_onCreate");
    if (!main)  {
        __android_log_print(ANDROID_LOG_ERROR, "dummyloader", "undefined symbol ANativeActivity_onCreate");
        exit(1);
    }

    main(app, ud, udsize);
}