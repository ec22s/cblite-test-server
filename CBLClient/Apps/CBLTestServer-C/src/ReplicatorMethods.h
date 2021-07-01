#pragma once

#include <nlohmann/json.hpp>
#include <civetweb.h>

namespace replicator_methods {
    void replicator_create(nlohmann::json& body, mg_connection* conn);
    void replicator_start(nlohmann::json& body, mg_connection* conn);
    void replicator_stop(nlohmann::json& body, mg_connection* conn);
    void replicator_status(nlohmann::json& body, mg_connection* conn);
    void replicator_getActivityLevel(nlohmann::json& body, mg_connection* conn);
    void replicator_getError(nlohmann::json& body, mg_connection* conn);
    void replicator_getTotal(nlohmann::json& body, mg_connection* conn);
    void replicator_getCompleted(nlohmann::json& body, mg_connection* conn);
    void replicator_addChangeListener(nlohmann::json& body, mg_connection* conn);
    void replicator_addReplicatorEventChangeListener(nlohmann::json& body, mg_connection* conn);
    void replicator_removeReplicatorEventListener(nlohmann::json& body, mg_connection* conn);
    void replicator_removeChangeListener(nlohmann::json& body, mg_connection* conn);
    void replicator_replicatorEventGetChanges(nlohmann::json& body, mg_connection* conn);
    void replicator_config(nlohmann::json& body, mg_connection* conn);
    void replicator_replicatorEventChangesCount(nlohmann::json& body, mg_connection* conn);
    void replicator_changeListenerChangesCount(nlohmann::json& body, mg_connection* conn);
}
