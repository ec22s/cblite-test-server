#pragma once

#include <civetweb.h>
#include <nlohmann/json.hpp>

namespace blob_methods {
    void blob_create(nlohmann::json& body, mg_connection* conn);
    void blob_createImageContent(nlohmann::json& body, mg_connection* conn);
    void blob_createImageStream(nlohmann::json& body, mg_connection* conn);
    void blob_createImageFileUrl(nlohmann::json& body, mg_connection* conn);
}