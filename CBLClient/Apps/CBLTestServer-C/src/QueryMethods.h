#pragma once

#include <nlohmann/json.hpp>
#include <civetweb.h>

namespace query_methods {
    void query_selectAll(nlohmann::json& body, mg_connection* conn);
    void query_addChangeListener(nlohmann::json& body, mg_connection* conn);
    void query_removeChangeListener(nlohmann::json& body, mg_connection* conn);
}
