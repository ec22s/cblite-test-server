#include "TestServer.h"
#include "Router.h"
#include "MemoryMap.h"
#include <civetweb.h>
#include <string>
#include <iostream>
#include <unistd.h>
using namespace std;

static int handle_request(mg_connection* connection, void* context) {
    const mg_request_info* request_info = mg_get_request_info(connection);
    if(strncmp("POST", request_info->request_method, 4) != 0) {
        mg_send_http_error(connection, 405, "%s", mg_get_response_code_text(connection, 405));
        return 405;
    }

    int status = 200;
    const string url = request_info->request_uri;
    try {
        router::internal::handle(url, connection);
    } catch(const exception& e) {
        mg_send_http_error(connection, 400, "Exception caught during router handling: %s", e.what());
        cerr << "Exception caught during router handling: " << e.what() << endl;
        status = 400;
    }

    mg_close_connection(connection);
    return status;
}

void TestServer::start() {
    string port_str = to_string(PORT);
    const char* options[3] = {"listening_ports", port_str.c_str(), nullptr};
    
    // Creating symbolic link to be able to load vector search, as the test app is archived without it.
    char cwd[1024];
    string currentDir = string(getcwd(cwd, sizeof(cwd)));
    string extensionsDir = currentDir + DIRECTORY_SEPARATOR + APP_EXTENSIONS_DIR;
    string symbolicLinkName = extensionsDir + DIRECTORY_SEPARATOR + "libgomp.so.1";
    string symbolicLinkOrigin = extensionsDir + DIRECTORY_SEPARATOR + "libgomp.so.1.0.0";
    symlink(symbolicLinkOrigin.c_str(), symbolicLinkName.c_str());
    memory_map::init();
    _httpContext = mg_start(nullptr, nullptr, options);
    mg_set_request_handler(_httpContext, "/*", handle_request, nullptr);
}

void TestServer::stop() {
    mg_stop(_httpContext);
    _httpContext = nullptr;
}
