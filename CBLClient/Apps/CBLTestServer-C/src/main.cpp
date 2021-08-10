#include "TestServer.h"
#include "ValueSerializer.h"
#include <thread>
#include <chrono>
#include <iostream>
#include <civetweb.h>

#include "Defines.h"
#include INCLUDE_CBL(CouchbaseLite.h)
using namespace std;
using namespace std::chrono;

int main(int argc, const char** argv) {
    mg_init_library(0);

    TestServer server{};
    server.start();
    cout << "Using CBL C version " << CBLITE_VERSION << "-" << CBLITE_BUILD_NUMBER << endl;
    cout << "Listening on port " << TestServer::PORT << "..." << endl;

    while(true) {
        std::this_thread::sleep_for(1s);
    }
}