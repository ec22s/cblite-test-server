#pragma once

#include <nlohmann/json.hpp>
#include <civetweb.h>

namespace dictionary_methods {
    void dictionary_contains(nlohmann::json& body, mg_connection* conn);
    void dictionary_count(nlohmann::json& body, mg_connection* conn);
    void dictionary_create(nlohmann::json& body, mg_connection* conn);
    void dictionary_getArray(nlohmann::json& body, mg_connection* conn);
    void dictionary_getBlob(nlohmann::json& body, mg_connection* conn);
    void dictionary_getBoolean(nlohmann::json& body, mg_connection* conn);
    void dictionary_getDate(nlohmann::json& body, mg_connection* conn);
    void dictionary_getDictionary(nlohmann::json& body, mg_connection* conn);
    void dictionary_getDouble(nlohmann::json& body, mg_connection* conn);
    void dictionary_getFloat(nlohmann::json& body, mg_connection* conn);
    void dictionary_getInt(nlohmann::json& body, mg_connection* conn);
    void dictionary_getKeys(nlohmann::json& body, mg_connection* conn);
    void dictionary_getLong(nlohmann::json& body, mg_connection* conn);
    void dictionary_getString(nlohmann::json& body, mg_connection* conn);
    void dictionary_getValue(nlohmann::json& body, mg_connection* conn);
    void dictionary_remove(nlohmann::json& body, mg_connection* conn);
    void dictionary_setArray(nlohmann::json& body, mg_connection* conn);
    void dictionary_setBlob(nlohmann::json& body, mg_connection* conn);
    void dictionary_setBoolean(nlohmann::json& body, mg_connection* conn);
    void dictionary_setDate(nlohmann::json& body, mg_connection* conn);
    void dictionary_setDictionary(nlohmann::json& body, mg_connection* conn);
    void dictionary_setDouble(nlohmann::json& body, mg_connection* conn);
    void dictionary_setFloat(nlohmann::json& body, mg_connection* conn);
    void dictionary_setInt(nlohmann::json& body, mg_connection* conn);
    void dictionary_setLong(nlohmann::json& body, mg_connection* conn);
    void dictionary_setString(nlohmann::json& body, mg_connection* conn);
    void dictionary_setValue(nlohmann::json& body, mg_connection* conn);
    void dictionary_toMap(nlohmann::json& body, mg_connection* conn);
    void dictionary_toMutableDictionary(nlohmann::json& body, mg_connection* conn);
}