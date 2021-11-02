#pragma once

#include <nlohmann/json.hpp>
#include <civetweb.h>

namespace file_logging_methods {
    void logging_configure(nlohmann::json& body, mg_connection* conn);
    void logging_getPlainTextStatus(nlohmann::json& body, mg_connection* conn);
    void logging_getMaxRotateCount(nlohmann::json& body, mg_connection* conn);
    void logging_getMaxSize(nlohmann::json& body, mg_connection* conn);
    void logging_getLogLevel(nlohmann::json& body, mg_connection* conn);
    void logging_getConfig(nlohmann::json& body, mg_connection* conn);
    void logging_getDirectory(nlohmann::json& body, mg_connection* conn);
    void logging_setPlainTextStatus(nlohmann::json& body, mg_connection* conn);
    void logging_setMaxRotateCount(nlohmann::json& body, mg_connection* conn);
    void logging_setMaxSize(nlohmann::json& body, mg_connection* conn);
    void logging_setLogLevel(nlohmann::json& body, mg_connection* conn);
    void logging_setConfig(nlohmann::json& body, mg_connection* conn);
    void logging_getLogsInZip(nlohmann::json& body, mg_connection* conn);
}