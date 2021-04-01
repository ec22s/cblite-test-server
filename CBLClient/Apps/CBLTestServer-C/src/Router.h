#pragma once

#include "ValueSerializer.h"

#include <string>
#include <civetweb.h>

struct mg_connection;

namespace router {
    namespace internal {
        void handle(std::string url, mg_connection* connection);
    }
}

template<typename T>
void write_serialized_body(mg_connection* conn, T object, bool success) {
    const string encoded = value_serializer::serialize(object);
    if(success) {
        mg_send_http_ok(conn, "application/json", encoded.size());
        mg_write(conn, encoded.c_str(), encoded.size());
    } else {
        mg_send_http_error(conn, 400, encoded.c_str());
    }
}