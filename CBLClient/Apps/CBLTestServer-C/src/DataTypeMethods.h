#pragma once

#include <nlohmann/json.hpp>
#include <civetweb.h>

namespace datatype_methods {
    // No idea why these are needed, inherited from port from .NET...
    void datatype_setLong(nlohmann::json& body, mg_connection* conn);
    void datatype_setDouble(nlohmann::json& body, mg_connection* conn);
    void datatype_setFloat(nlohmann::json& body, mg_connection* conn);
    void datatype_setDate(nlohmann::json& body, mg_connection* conn);

    void datatype_compare(nlohmann::json& body, mg_connection* conn);
    void datatype_compareLong(nlohmann::json& body, mg_connection* conn);
    void datatype_compareDouble(nlohmann::json& body, mg_connection* conn);
    void datatype_compareDate(nlohmann::json& body, mg_connection* conn);
}
