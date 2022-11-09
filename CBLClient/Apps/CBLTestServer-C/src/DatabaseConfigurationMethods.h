#pragma once

#include <nlohmann/json.hpp>
#include <civetweb.h>

namespace database_configuration_methods {
    void database_configuration_configure(nlohmann::json& body, mg_connection* conn);
    void database_configuration_setEncryptionKey(nlohmann::json& body, mg_connection* conn);
    void database_configuration_configure_old(nlohmann::json& body, mg_connection* conn);
}
