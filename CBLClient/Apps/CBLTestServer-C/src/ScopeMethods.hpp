#pragma once

#include <nlohmann/json.hpp>
#include <civetweb.h>

namespace scope_methods {

    void scope_scopeName(nlohmann::json& body, mg_connection* conn);
    void scope_collectionName(nlohmann::json& body, mg_connection* conn);
    void scope_collection(nlohmann::json& body, mg_connection* conn);
}
