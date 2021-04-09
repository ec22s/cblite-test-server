#include "Router.h"
#include "MemoryMap.h"
#include "ArrayMethods.h"
#include "DictionaryMethods.h"
#include "DocumentMethods.h"
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
    { "dictionary_contains", dictionary_methods::dictionary_contains },
    { "dictionary_count", dictionary_methods::dictionary_count },
    { "dictionary_create", dictionary_methods::dictionary_create },
    { "dictionary_getArray", dictionary_methods::dictionary_getArray },
    { "dictionary_getBlob", dictionary_methods::dictionary_getBlob },
    { "dictionary_getBoolean", dictionary_methods::dictionary_getBoolean },
    { "dictionary_getDate", dictionary_methods::dictionary_getDate },
    { "dictionary_getDictionary", dictionary_methods::dictionary_getDictionary },
    { "dictionary_getDouble", dictionary_methods::dictionary_getDouble },
    { "dictionary_getFloat", dictionary_methods::dictionary_getFloat },
    { "dictionary_getInt", dictionary_methods::dictionary_getInt },
    { "dictionary_getKeys", dictionary_methods::dictionary_getKeys },
    { "dictionary_getLong", dictionary_methods::dictionary_getLong },
    { "dictionary_getString", dictionary_methods::dictionary_getString },
    { "dictionary_remove", dictionary_methods::dictionary_remove },
    { "dictionary_setArray", dictionary_methods::dictionary_setArray },
    { "dictionary_setBlob", dictionary_methods::dictionary_setBlob },
    { "dictionary_setBoolean", dictionary_methods::dictionary_setBoolean },
    { "dictionary_setDate", dictionary_methods::dictionary_setDate },
    { "dictionary_setDictionary", dictionary_methods::dictionary_setDictionary },
    { "dictionary_setDouble", dictionary_methods::dictionary_setDouble },
    { "dictionary_setFloat", dictionary_methods::dictionary_setFloat },
    { "dictionary_setInt", dictionary_methods::dictionary_setInt },
    { "dictionary_setLong", dictionary_methods::dictionary_setLong },
    { "dictionary_getValue", dictionary_methods::dictionary_getValue },
    { "dictionary_setString", dictionary_methods::dictionary_setString },
    { "dictionary_setValue", dictionary_methods::dictionary_setValue },
    { "dictionary_toMap", dictionary_methods::dictionary_toMap },
    { "dictionary_toMutableDictionary", dictionary_methods::dictionary_toMutableDictionary },
    { "document_create", document_methods::document_create },
    { "document_delete", document_methods::document_delete },
    { "document_getId", document_methods::document_getId },
    { "document_getString", document_methods::document_getString },
    { "document_setString", document_methods::document_setString },
    { "document_count", document_methods::document_count },
    { "document_getInt", document_methods::document_getInt },
    { "document_setInt", document_methods::document_setInt },
    { "document_getLong", document_methods::document_getLong },
    { "document_setLong", document_methods::document_setLong },
    { "document_getFloat", document_methods::document_getFloat },
    { "document_setFloat", document_methods::document_setFloat },
    { "document_getDouble", document_methods::document_getDouble },
    { "document_setDouble", document_methods::document_setDouble },
    { "document_getBoolean", document_methods::document_getBoolean },
    { "document_setBoolean", document_methods::document_setBoolean },
    { "document_getBlob", document_methods::document_getBlob },
    { "document_setBlob", document_methods::document_setBlob },
    { "document_getArray", document_methods::document_getArray },
    { "document_setArray", document_methods::document_setArray },
    { "document_getDate", document_methods::document_getDate },
    { "document_setDate", document_methods::document_setDate },
    { "document_setData", document_methods::document_setData },
    { "document_getDictionary", document_methods::document_getDictionary },
    { "document_setDictionary", document_methods::document_setDictionary },
    { "document_getKeys", document_methods::document_getKeys },
    { "document_toMap", document_methods::document_toMap },
    { "document_toMutable", document_methods::document_toMutable },
    { "document_removeKey", document_methods::document_removeKey },
    { "document_contains", document_methods::document_contains },
    { "document_getValue", document_methods::document_getValue },
    { "document_setValue", document_methods::document_setValue },
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