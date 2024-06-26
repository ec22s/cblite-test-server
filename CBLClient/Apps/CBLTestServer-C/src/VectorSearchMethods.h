#pragma once

#include <nlohmann/json.hpp>
#include <civetweb.h>

namespace vectorSearch_methods
{
    void vectorSearch_createIndex(nlohmann::json &body, mg_connection *conn);
    void vectorSearch_loadDatabase(nlohmann::json &body, mg_connection *conn);
    void vectorSearch_registerModel(nlohmann::json &body, mg_connection *conn);
    void vectorSearch_query(nlohmann::json &body, mg_connection *conn);
    void vectorSearch_getEmbedding(nlohmann::json &body, mg_connection *conn);
}