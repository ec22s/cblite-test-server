#include "Router.h"
#include "MemoryMap.h"
#include "ArrayMethods.h"
#include "ValueSerializer.h"
#include <functional>
#include <utility>
#include <civetweb.h>
#include <unordered_map>
#include <nlohmann/json.hpp>
#include <sstream>

using namespace std;
using namespace nlohmann;

using endpoint_handler = function<void(json&, mg_connection*)>;

static void releaseObject(json& body, mg_connection* conn) {
    const auto id = body["object"].get<string>();
    memory_map::release(id);
    mg_send_http_ok(conn, nullptr, 0);
}

static void flushMemory(json& body, mg_connection* conn) {
    memory_map::clear();
    mg_send_http_ok(conn, nullptr, 0);
}

static const unordered_map<string, endpoint_handler> ROUTE_MAP = {
    { "array_addDictionary", array_methods::array_addDictionary },
    { "array_addString", array_methods::array_addString },
    { "array_create", array_methods::array_create },
    { "array_getArray", array_methods::array_getArray },
    { "array_getDictionary", array_methods::array_getDictionary },    
    { "array_getString", array_methods::array_getString },
    { "release", releaseObject },
    { "flushMemory", flushMemory },
};

void router::internal::handle(string url, mg_connection* connection) {
    url.erase(0, url.find_first_not_of('/'));
    const auto& handler = ROUTE_MAP.find(url);
    if(handler == ROUTE_MAP.end()) {
        mg_send_http_error(connection, 404, mg_get_response_code_text(connection, 404));
        return;
    }

    stringstream s;
    char buf[8192];
    int r = mg_read(connection, buf, 8192);
    while(r > 0) {
        s.write(buf, r);
        r = mg_read(connection, buf, 8192);
    }

    json body;
    if(s.tellp() >= 2) {
        s >> body;
    }

    handler->second(body, connection);
}