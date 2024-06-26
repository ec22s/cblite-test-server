
#pragma once

#include <nlohmann/json.hpp>
#include <civetweb.h>

namespace collection_methods {
    void collection_defaultCollection(nlohmann::json& body, mg_connection* conn);
    void collection_documentCount(nlohmann::json& body, mg_connection* conn);
    void collection_createCollection(nlohmann::json& body, mg_connection* conn);
    void collection_deleteCollection(nlohmann::json& body, mg_connection* conn);
    void collection_collectionNames(nlohmann::json& body, mg_connection* conn);
    void collection_getCollectionName(nlohmann::json& body, mg_connection* conn);
    void collection_deleteCollection(nlohmann::json& body, mg_connection* conn);
    void collection_updateDocument(nlohmann::json& body, mg_connection* conn);
    void collection_allScope(nlohmann::json& body, mg_connection* conn);
    void collection_scope(nlohmann::json& body, mg_connection* conn);
    void collection_collection(nlohmann::json& body, mg_connection* conn);
    void collection_collectionScope(nlohmann::json& body, mg_connection* conn);
    void collection_getDocument(nlohmann::json& body, mg_connection* conn);
    void collection_getDocuments(nlohmann::json& body, mg_connection* conn);
    void collection_saveDocument(nlohmann::json& body, mg_connection* conn);
    void collection_saveDocumentWithConcurrencyControl(nlohmann::json& body, mg_connection* conn);
    void collection_deleteDocument(nlohmann::json& body, mg_connection* conn);
    void collection_deleteDocumentWithConcurrencyControl(nlohmann::json& body, mg_connection* conn);
    void collection_purgeDocument(nlohmann::json& body, mg_connection* conn);
    void collection_saveDocuments(nlohmann::json& body, mg_connection* conn);
    void collection_purgeDocumentID(nlohmann::json& body, mg_connection* conn);
    void collection_getDocumentExpiration(nlohmann::json& body, mg_connection* conn);
    void collection_setDocumentExpiration(nlohmann::json& body, mg_connection* conn);
    void collection_getMutableDocument(nlohmann::json& body, mg_connection* conn);
    void collection_createValueIndex(nlohmann::json& body, mg_connection* conn);
    void collection_deleteIndex(nlohmann::json& body, mg_connection* conn);
    void collection_getIndexNames(nlohmann::json& body, mg_connection* conn);
    void collection_addChangeListener(nlohmann::json& body, mg_connection* conn);
    void collection_addDocumentChangeListener(nlohmann::json& body, mg_connection* conn);
}
