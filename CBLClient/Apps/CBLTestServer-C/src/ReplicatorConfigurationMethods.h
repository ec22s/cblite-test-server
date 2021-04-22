#pragma once

#include <nlohmann/json.hpp>
#include <civetweb.h>

namespace replicator_configuration_methods {
    void replicatorConfiguration_create(nlohmann::json& body, mg_connection* conn);
    void replicatorConfiguration_setAuthenticator(nlohmann::json& body, mg_connection* conn);
    void replicatorConfiguration_setReplicatorType(nlohmann::json& body, mg_connection* conn);
    void replicatorConfiguration_isContinuous(nlohmann::json& body, mg_connection* conn);
}