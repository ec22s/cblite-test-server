#pragma once

#include <nlohmann/json.hpp>
#include <civetweb.h>

namespace session_authentication_methods {
    void session_authentication_create(nlohmann::json& body, mg_connection* conn);
}