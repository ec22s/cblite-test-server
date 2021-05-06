#pragma once

#include <condition_variable>

struct mg_context;

class TestServer {
public:
    static constexpr uint16_t PORT = 1234;
    void start();
    void stop();

private:
    mg_context* _httpContext;
};