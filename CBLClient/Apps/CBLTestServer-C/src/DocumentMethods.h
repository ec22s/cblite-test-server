#pragma once

#include <nlohmann/json.hpp>
#include <civetweb.h>

namespace document_methods {
    void document_create(nlohmann::json& body, mg_connection* conn);
    void document_delete(nlohmann::json& body, mg_connection* conn);
    void document_getId(nlohmann::json& body, mg_connection* conn);
    void document_getString(nlohmann::json& body, mg_connection* conn);
    void document_setString(nlohmann::json& body, mg_connection* conn);
    void document_count(nlohmann::json& body, mg_connection* conn);
    void document_getInt(nlohmann::json& body, mg_connection* conn);
    void document_setInt(nlohmann::json& body, mg_connection* conn);
    void document_getLong(nlohmann::json& body, mg_connection* conn);
    void document_setLong(nlohmann::json& body, mg_connection* conn);
    void document_getFloat(nlohmann::json& body, mg_connection* conn);
    void document_setFloat(nlohmann::json& body, mg_connection* conn);
    void document_getDouble(nlohmann::json& body, mg_connection* conn);
    void document_setDouble(nlohmann::json& body, mg_connection* conn);
    void document_getBoolean(nlohmann::json& body, mg_connection* conn);
    void document_setBoolean(nlohmann::json& body, mg_connection* conn);
    void document_getBlob(nlohmann::json& body, mg_connection* conn);
    void document_setBlob(nlohmann::json& body, mg_connection* conn);
    void document_getArray(nlohmann::json& body, mg_connection* conn);
    void document_setArray(nlohmann::json& body, mg_connection* conn);
    void document_getDate(nlohmann::json& body, mg_connection* conn);
    void document_setDate(nlohmann::json& body, mg_connection* conn);
    void document_setData(nlohmann::json& body, mg_connection* conn);
    void document_getDictionary(nlohmann::json& body, mg_connection* conn);
    void document_setDictionary(nlohmann::json& body, mg_connection* conn);
    void document_getKeys(nlohmann::json& body, mg_connection* conn);
    void document_toMap(nlohmann::json& body, mg_connection* conn);
    void document_toMutable(nlohmann::json& body, mg_connection* conn);
    void document_removeKey(nlohmann::json& body, mg_connection* conn);
    void document_contains(nlohmann::json& body, mg_connection* conn);
    void document_getValue(nlohmann::json& body, mg_connection* conn);
    void document_setValue(nlohmann::json& body, mg_connection* conn);
}