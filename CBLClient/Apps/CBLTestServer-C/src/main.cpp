#include "TestServer.h"
#include "ValueSerializer.h"
#include <thread>
#include <chrono>
#include <iostream>
#include <civetweb.h>
using namespace std;
using namespace std::chrono;

int main(int argc, const char** argv) {
    mg_init_library(0);

    TestServer server{};
    server.start();
    std::cout << "Listening on port " << TestServer::PORT << "...";

    while(true) {
        std::this_thread::sleep_for(1s);
    }
}