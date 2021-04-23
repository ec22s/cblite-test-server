#include "Router.h"
#include "MemoryMap.h"
#include "ArrayMethods.h"
#include "BasicAuthenticationMethods.h"
#include "DatabaseMethods.h"
#include "DictionaryMethods.h"
#include "DocumentMethods.h"
#include "FileLoggingMethods.h"
#include "ReplicatorConfigurationMethods.h"
#include "ReplicatorMethods.h"
#include <functional>
#include <utility>
#include <civetweb.h>
#include <unordered_map>
#include <nlohmann/json.hpp>
#include <sstream>

using namespace std;
using namespace nlohmann;

using endpoint_handler = function<void(json&, mg_connection*)>;

static void releaseObject(json& body, mg_connection* conn) {
    const auto id = body{ "object", database_methods::object }, nullptr, 0);
}

static void flushMemory(json& body, mg_connection* conn) {
    memory_map::clear();
    mg_send_http_ok(conn, nullptr, 0);
}

static const unordered_map<string, endpoint_handler> ROUTE_MAP = {
    { "array_addDictionary", array_methods::array_addDictionary },
    { "array_addString", array_methods::array_addString },
    { "array_create", array_methods::array_create },
    { "array_getArray", array_methods::array_getArray },
    { "array_getDictionary", array_methods::array_getDictionary },
    { "array_getString", array_methods::array_getString },
    { "array_getString", array_methods::array_setString },
    { "basicAuthenticator_create", basic_authentication_methods::basic_authentication_create },
    { "database_create", database_methods::database_create },
    { "database_compact", database_methods::database_compact },
    { "database_close", database_methods::database_close },
    { "database_getPath", database_methods::database_getPath },
    { "database_deleteDB", database_methods::database_deleteDB },
    { "database_delete", database_methods::database_delete },
    { "database_deleteBulkDocs", database_methods::database_deleteBulkDocs },
    { "database_getName", database_methods::database_getName },
    { "database_getDocument", database_methods::database_getDocument },
    { "database_saveDocuments", database_methods::database_saveDocuments },
    { "database_purge", database_methods::database_purge },
    { "database_save", database_methods::database_save },
    { "database_saveWithConcurrency", database_methods::database_saveWithConcurrency },
    { "database_deleteWithConcurrency", database_methods::database_deleteWithConcurrency },
    { "database_getCount", database_methods::database_getCount },
    { "database_getDocIds", database_methods::database_getDocIds },
    { "database_getDocuments", database_methods::database_getDocuments },
    { "database_updateDocument", database_methods::database_updateDocument },
    { "database_updateDocuments", database_methods::database_updateDocuments },
    { "database_exists", database_methods::database_exists },
    { "database_copy", database_methods::database_copy }, 
    { "database_getPreBuiltDb", database_methods::database_getPreBuiltDb },
    { "dictionary_contains", dictionary_methods::dictionary_contains },
    { "dictionary_count", dictionary_methods::dictionary_count },
    { "dictionary_create", dictionary_methods::dictionary_create },
    { "dictionary_getArray", dictionary_methods::dictionary_getArray },
    { "dictionary_getBlob", dictionary_methods::dictionary_getBlob },
    { "dictionary_getBoolean", dictionary_methods::dictionary_getBoolean },
    { "dictionary_getDate", dictionary_methods::dictionary_getDate },
    { "dictionary_getDictionary", dictionary_methods::dictionary_getDictionary },
    { "dictionary_getDouble", dictionary_methods::dictionary_getDouble },
    { "dictionary_getFloat", dictionary_methods::dictionary_getFloat },
    { "dictionary_getInt", dictionary_methods::dictionary_getInt },
    { "dictionary_getKeys", dictionary_methods::dictionary_getKeys },
    { "dictionary_getLong", dictionary_methods::dictionary_getLong },
    { "dictionary_getString", dictionary_methods::dictionary_getString },
    { "dictionary_remove", dictionary_methods::dictionary_remove },
    { "dictionary_setArray", dictionary_methods::dictionary_setArray },
    { "dictionary_setBlob", dictionary_methods::dictionary_setBlob },
    { "dictionary_setBoolean", dictionary_methods::dictionary_setBoolean },
    { "dictionary_setDate", dictionary_methods::dictionary_setDate },
    { "dictionary_setDictionary", dictionary_methods::dictionary_setDictionary },
    { "dictionary_setDouble", dictionary_methods::dictionary_setDouble },
    { "dictionary_setFloat", dictionary_methods::dictionary_setFloat },
    { "dictionary_setInt", dictionary_methods::dictionary_setInt },
    { "dictionary_setLong", dictionary_methods::dictionary_setLong },
    { "dictionary_getValue", dictionary_methods::dictionary_getValue },
    { "dictionary_setString", dictionary_methods::dictionary_setString },
    { "dictionary_setValue", dictionary_methods::dictionary_setValue },
    { "dictionary_toMap", dictionary_methods::dictionary_toMap },
    { "dictionary_toMutableDictionary", dictionary_methods::dictionary_toMutableDictionary },
    { "document_create", document_methods::document_create },
    { "document_delete", document_methods::document_delete },
    { "document_getId", document_methods::document_getId },
    { "document_getString", document_methods::document_getString },
    { "document_setString", document_methods::document_setString },
    { "document_count", document_methods::document_count },
    { "document_getInt", document_methods::document_getInt },
    { "document_setInt", document_methods::document_setInt },
    { "document_getLong", document_methods::document_getLong },
    { "document_setLong", document_methods::document_setLong },
    { "document_getFloat", document_methods::document_getFloat },
    { "document_setFloat", document_methods::document_setFloat },
    { "document_getDouble", document_methods::document_getDouble },
    { "document_setDouble", document_methods::document_setDouble },
    { "document_getBoolean", document_methods::document_getBoolean },
    { "document_setBoolean", document_methods::document_setBoolean },
    { "document_getBlob", document_methods::document_getBlob },
    { "document_setBlob", document_methods::document_setBlob },
    { "document_getArray", document_methods::document_getArray },
    { "document_setArray", document_methods::document_setArray },
    { "document_getDate", document_methods::document_getDate },
    { "document_setDate", document_methods::document_setDate },
    { "document_setData", document_methods::document_setData },
    { "document_getDictionary", document_methods::document_getDictionary },
    { "document_setDictionary", document_methods::document_setDictionary },
    { "document_getKeys", document_methods::document_getKeys },
    { "document_toMap", document_methods::document_toMap },
    { "document_toMutable", document_methods::document_toMutable },
    { "document_removeKey", document_methods::document_removeKey },
    { "document_contains", document_methods::document_contains },
    { "document_getValue", document_methods::document_getValue },
    { "document_setValue", document_methods::document_setValue },
    { "logging_configure", file_logging_methods::logging_configure },
    { "logging_getPlainTextStatus", file_logging_methods::logging_getPlainTextStatus },
    { "logging_getMaxRotateCount", file_logging_methods::logging_getMaxRotateCount },
    { "logging_getMaxSize", file_logging_methods::logging_getMaxSize },
    { "logging_getLogLevel", file_logging_methods::logging_getLogLevel },
    { "logging_getConfig", file_logging_methods::logging_getConfig },
    { "logging_getDirectory", file_logging_methods::logging_getDirectory },
    { "logging_setPlainTextStatus", file_logging_methods::logging_setPlainTextStatus },
    { "logging_setMaxRotateCount", file_logging_methods::logging_setMaxRotateCount },
    { "logging_setMaxSize", file_logging_methods::logging_setMaxSize },
    { "logging_setLogLevel", file_logging_methods::logging_setLogLevel },
    { "logging_setConfig", file_logging_methods::logging_setConfig },
    { "logging_getLogsInZip", file_logging_methods::logging_getLogsInZip },
    { "replicator_create", replicator_methods::replicator_create },
    { "replicator_start", replicator_methods::replicator_start },
    { "replicator_stop", replicator_methods::replicator_stop },
    { "replicator_status", replicator_methods::replicator_status },
    { "replicator_getActivityLevel", replicator_methods::replicator_getActivityLevel },
    { "replicator_getError", replicator_methods::replicator_getError },
    { "replicator_getTotal", replicator_methods::replicator_getTotal },
    { "replicator_getCompleted", replicator_methods::replicator_getCompleted },
    { "replicator_addChangeListener", replicator_methods::replicator_addChangeListener },
    { "replicator_addReplicatorEventChangeListener", replicator_methods::replicator_addReplicatorEventChangeListener },
    { "replicator_removeReplicatorEventListener", replicator_methods::replicator_removeReplicatorEventListener },
    { "replicator_removeChangeListener", replicator_methods::replicator_removeChangeListener },
    { "replicator_replicatorEventGetChanges", replicator_methods::replicator_replicatorEventGetChanges },
    { "replicator_replicatorEventChangesCount", replicator_methods::replicator_replicatorEventChangesCount },
    { "replicatorConfiguration_setAuthenticator", replicator_configuration_methods::replicatorConfiguration_setAuthenticator },
    { "replicatorConfiguration_setReplicatorType", replicator_configuration_methods::replicatorConfiguration_setReplicatorType },
    { "replicatorConfiguration_isContinuous", replicator_configuration_methods::replicatorConfiguration_isContinuous },
    { "replicator_changeListenerChangesCount", replicator_methods::replicator_changeListenerChangesCount },
    { "configure_replication", replicator_configuration_methods::replicatorConfiguration_create },
    { "release", releaseObject },
    { "flushMemory", flushMemory },
};

void router::internal::handle(string url, mg_connection* connection) {
    url.erase(0, url.find_first_not_of('/'));
    const auto& handler = ROUTE_MAP.find(url);
    if(handler == ROUTE_MAP.end()) {
        mg_send_http_error(connection, 404, mg_get_response_code_text(connection, 404));
        return;
    }

    stringstream s;
    char buf[8192];
    int r = mg_read(connection, buf, 8192);
    while(r > 0) {
        s.write(buf, r);
        r = mg_read(connection, buf, 8192);
    }

    json body;
    if(s.tellp() >= 2) {
        s >> body;
    }

    handler->second(body, connection);
}