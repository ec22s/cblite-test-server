#pragma once

#include <nlohmann/json.hpp>
#include <civetweb.h>

namespace database_configuration_methods {
    void database_configuration_configure(nlohmann::json& body, mg_connection* conn);
}