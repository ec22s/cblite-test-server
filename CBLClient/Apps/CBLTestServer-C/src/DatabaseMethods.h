#pragma once

#include <nlohmann/json.hpp>
#include <civetweb.h>

namespace database_methods {
    void database_create(nlohmann::json& body, mg_connection* conn);
    void database_compact(nlohmann::json& body, mg_connection* conn);
    void database_close(nlohmann::json& body, mg_connection* conn);
    void database_getPath(nlohmann::json& body, mg_connection* conn);
    void database_deleteDB(nlohmann::json& body, mg_connection* conn);
    void database_delete(nlohmann::json& body, mg_connection* conn);
    void database_deleteBulkDocs(nlohmann::json& body, mg_connection* conn);
    void database_getName(nlohmann::json& body, mg_connection* conn);
    void database_getDocument(nlohmann::json& body, mg_connection* conn);
    void database_saveDocuments(nlohmann::json& body, mg_connection* conn);    
    void database_purge(nlohmann::json& body, mg_connection* conn);
    void database_save(nlohmann::json& body, mg_connection* conn);
    void database_saveWithConcurrency(nlohmann::json& body, mg_connection* conn);
    void database_deleteWithConcurrency(nlohmann::json& body, mg_connection* conn);
    void database_getCount(nlohmann::json& body, mg_connection* conn);
    void database_getDocIds(nlohmann::json& body, mg_connection* conn);
    void database_getDocuments(nlohmann::json& body, mg_connection* conn);
    void database_updateDocument(nlohmann::json& body, mg_connection* conn);
    void database_updateDocuments(nlohmann::json& body, mg_connection* conn);
    void database_exists(nlohmann::json& body, mg_connection* conn);
    void database_copy(nlohmann::json& body, mg_connection* conn);
    void database_getPreBuiltDb(nlohmann::json& body, mg_connection* conn);
}