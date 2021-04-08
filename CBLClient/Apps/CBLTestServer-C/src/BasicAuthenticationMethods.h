#pragma once

#include <nlohmann/json.hpp>
#include <civetweb.h>

namespace basic_authentication_methods {
    void basic_authentication_create(nlohmann::json& body, mg_connection* conn);
}