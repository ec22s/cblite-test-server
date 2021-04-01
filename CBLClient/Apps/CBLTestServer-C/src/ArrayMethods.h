#pragma once

#include <nlohmann/json.hpp>
#include <civetweb.h>

namespace array_methods {
    void array_addDictionary(nlohmann::json& body, mg_connection* conn);
    void array_addString(nlohmann::json& body, mg_connection* conn);
    void array_create(nlohmann::json& body, mg_connection* conn);
    void array_getArray(nlohmann::json& body, mg_connection* conn);
    void array_getDictionary(nlohmann::json& body, mg_connection* conn);
    void array_getString(nlohmann::json& body, mg_connection* conn);
    void array_setArray(nlohmann::json& body, mg_connection* conn);
    void array_setDictionary(nlohmann::json& body, mg_connection* conn);
}