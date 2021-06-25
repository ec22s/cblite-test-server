#include "FileLoggingMethods.h"
#include "Router.h"
#include "MemoryMap.h"
#include "Defines.h"

#include <iostream>
#include <string>
#include <sstream>
#include <dirent.h>
#include <zip.h>
#include <fstream>
#include INCLUDE_CBL(CouchbaseLite.h)


#ifdef _MSC_VER
#include <atlbase.h>
typedef struct _stat64 cbl_stat_t;
#define cbl_stat _stat64
#else
#include <sys/stat.h>
typedef struct stat cbl_stat_t;
#define cbl_stat stat
#endif

using namespace std;
using namespace nlohmann;

static string LogTempDirectory() {
#ifdef _MSC_VER
    WCHAR pathBuffer[MAX_PATH + 1];
    GetTempPathW(MAX_PATH, pathBuffer);
    GetLongPathNameW(pathBuffer, pathBuffer, MAX_PATH);
    CW2AEX<256> convertedPath(pathBuffer, CP_UTF8);
    return string(convertedPath.m_psz);
#elif defined(__ANDROID__)
    return "/data/local/tmp"; // This is our internal program, let's hope this path works reliably
#else
    return "/tmp";
#endif
}

static json serialize_config(const CBLLogFileConfiguration *config) {
    json serialized;
    serialized["usePlaintext"] = config->usePlaintext;
    serialized["maxSize"] = config->maxSize;
    serialized["maxRotateCount"] = config->maxRotateCount;
    serialized["level"] = config->level;
    serialized["directory"] = string(static_cast<const char*>(config->directory.buf), config->directory.size);
    return serialized;
}

static bool is_dir(const struct dirent *entry, const string &basePath) {
        bool isDir = false;
#ifdef _DIRENT_HAVE_D_TYPE
        if(entry->d_type != DT_UNKNOWN && entry->d_type != DT_LNK) {
            isDir = (entry->d_type == DT_DIR);
        } else
#endif
        {
            cbl_stat_t stbuf;
            cbl_stat((basePath + entry->d_name).c_str(), &stbuf);
            isDir = S_ISDIR(stbuf.st_mode);
        }

        return isDir;
    }

namespace file_logging_methods {
    void logging_configure(json& body, mg_connection* conn) {
        string directory;
        if(!body.contains("directory")) {
            stringstream ss;
            ss << LogTempDirectory() << DIRECTORY_SEPARATOR << "log_" << CBL_Now();
            directory = ss.str();
            cout << "File logging configured at: " << directory;
            cbl_mkdir(directory.c_str(), 0755);
        } else {
            directory = body["directory"].get<string>();
        }

        CBLLogFileConfiguration config {};
        config.directory = { directory.data(), directory.size() };
        if(body.contains("max_rotate_count")) {
            const auto max_rotate_count = body["max_rotate_count"].get<int>();
            config.maxRotateCount = max_rotate_count;
        }

        if(body.contains("max_size")) {
            const auto max_size = body["max_size"].get<int64_t>();
            config.maxSize = max_size;
        }

        if(body.contains("plain_text")) {
            const auto plain_text = body["plain_text"].get<bool>();
            config.usePlaintext = plain_text;
        }

        if(body.contains("log_level")) {
            const auto logLevel = body["log_level"].get<string>();
            if(logLevel == "debug") {
                config.level = CBLLogDebug;
            } else if(logLevel == "verbose") {
                config.level = CBLLogVerbose;
            } else if(logLevel == "info") {
                config.level = CBLLogInfo;
            } else if(logLevel == "warning") {
                config.level = CBLLogWarning;
            } else if(logLevel == "error") {
                config.level = CBLLogError;
            } else {
                config.level = CBLLogNone;
            }
        }

        CBLError err;
        TRY(CBLLog_SetFileConfig(config, &err), err)
        mg_send_http_ok(conn, "text/plain", 0);
    }

    void logging_getPlainTextStatus(json& body, mg_connection* conn) {
        const auto* config = CBLLog_FileConfig();
        write_serialized_body(conn, config->usePlaintext);
    }

    void logging_getMaxRotateCount(json& body, mg_connection* conn) {
        const auto* config = CBLLog_FileConfig();
        write_serialized_body(conn, config->maxRotateCount);
    }

    void logging_getMaxSize(json& body, mg_connection* conn) {
        const auto* config = CBLLog_FileConfig();
        write_serialized_body(conn, config->maxSize);
    }

    void logging_getLogLevel(json& body, mg_connection* conn) {
        const auto* config = CBLLog_FileConfig();
        write_serialized_body(conn, static_cast<int>(config->level));
    }

    void logging_getConfig(json& body, mg_connection* conn) {
        const auto* config = CBLLog_FileConfig();
        json serialized = serialize_config(config);
        write_serialized_body(conn, serialized);
    }

    void logging_getDirectory(json& body, mg_connection* conn) {
        const auto* config = CBLLog_FileConfig();
        write_serialized_body(conn,  string(static_cast<const char*>(config->directory.buf), config->directory.size));
    }

    void logging_setPlainTextStatus(json& body, mg_connection* conn) {
        const auto plaintext = body["plain_text"].get<bool>();
        CBLLogFileConfiguration config = *CBLLog_FileConfig();
        config.usePlaintext = plaintext;

        CBLError err;
        TRY(CBLLog_SetFileConfig(config, &err), err)
        json serialized = serialize_config(&config);
        write_serialized_body(conn, serialized);
    }

    void logging_setMaxRotateCount(json& body, mg_connection* conn) {
        const auto maxRotateCount = body["max_rotate_count"].get<int>();
        CBLLogFileConfiguration config = *CBLLog_FileConfig();
        config.maxRotateCount = maxRotateCount;

        CBLError err;
        TRY(CBLLog_SetFileConfig(config, &err), err)
        json serialized = serialize_config(&config);
        write_serialized_body(conn, serialized);
    }

    void logging_setMaxSize(json& body, mg_connection* conn) {
        const auto maxSize = body["max_size"].get<int64_t>();
        CBLLogFileConfiguration config = *CBLLog_FileConfig();
        config.maxSize = maxSize;

        CBLError err;
        TRY(CBLLog_SetFileConfig(config, &err), err)
        json serialized = serialize_config(&config);
        write_serialized_body(conn, serialized);
    }

    void logging_setLogLevel(json& body, mg_connection* conn) {
        const auto level = body["log_level"].get<string>();
        CBLLogFileConfiguration config = *CBLLog_FileConfig();
        if(level == "debug") {
            config.level = CBLLogDebug;
        } else if(level == "verbose") {
            config.level = CBLLogVerbose;
        } else if(level == "info") {
            config.level = CBLLogInfo;
        } else if(level == "error") {
            config.level = CBLLogError;
        } else if(level == "warning") {
            config.level = CBLLogWarning;
        } else {
            config.level = CBLLogNone;
        }

        CBLError err;
        TRY(CBLLog_SetFileConfig(config, &err), err)
        json serialized = serialize_config(&config);
        write_serialized_body(conn, serialized);
    }

    void logging_setConfig(json& body, mg_connection* conn) {
        auto directory = body["directory"].get<string>();
        if(directory.empty()) {
            stringstream ss;
            ss << LogTempDirectory() << DIRECTORY_SEPARATOR << "log_" << CBL_Now();
            directory = ss.str();
            cout << "File logging configured at: " << directory;
            cbl_mkdir(directory.c_str(), 0755);
        }

        auto config = *CBLLog_FileConfig();
        config.directory = { directory.data(), directory.size() };
        CBLError err;
        TRY(CBLLog_SetFileConfig(config, &err), err)
        json serialized = serialize_config(&config);
        write_serialized_body(conn, serialized);
    }

    void logging_getLogsInZip(json& body, mg_connection* conn) {
        auto flDir = CBLLog_FileConfig()->directory;
        string logDirectory(static_cast<const char *>(flDir.buf), flDir.size);
        DIR* dir = opendir(logDirectory.c_str());
        if(!dir) {
            mg_send_http_error(conn, 500, "opendir returned error %d", errno);
            return;
        }

        zip_error_t zErr;
        zip_source_t* zipSrc = zip_source_buffer_create(nullptr, 0, 0, &zErr);
        if(!zipSrc) {
            mg_send_http_error(conn, 500, "%s", zErr.str);
            zip_error_fini(&zErr);
            return;
        }

        zip_t* za = zip_open_from_source(zipSrc, ZIP_TRUNCATE, &zErr);
        if(!za) {
            mg_send_http_error(conn, 500, "%s", zErr.str);
            zip_error_fini(&zErr);
            return;
        }


        while(true) {
            struct dirent* contents = readdir(dir);
            if(!contents) {
                break;
            }

            string name(contents->d_name);
            if(!name.empty() && name[0] != '.' && !is_dir(contents, logDirectory)) {
                string fullPath = logDirectory + DIRECTORY_SEPARATOR + name;
                FILE* fp;
                int openResult = cbl_fopen(&fp, fullPath.c_str(), "rb");
                if(openResult != 0) {
                    cerr << "Unable to open " << name << endl;
                    continue;
                }

                zip_source_t* f = zip_source_filep_create(fp, 0, -1, &zErr);
                if(!f) {
                    cerr << "Unable to read " << name << endl;
                    continue;
                }

                if(zip_file_add(za, contents->d_name, f, ZIP_FL_ENC_UTF_8) < 0) {
                    cerr << "Unable to compress " << name << " (" << zip_strerror(za) << ")" << endl;
                    zip_source_free(f);
                }
            }
        }

        zip_source_keep(zipSrc);
        zip_close(za);
        zip_source_open(zipSrc);
        zip_source_seek(zipSrc, 0, SEEK_END);
        zip_int64_t sz = zip_source_tell(zipSrc);
        zip_source_seek(zipSrc, 0, SEEK_SET);
        void* outbuffer = malloc(sz);
        zip_source_read(zipSrc, outbuffer, sz);
        mg_send_http_ok(conn, "application/zip", sz);
        mg_write(conn, outbuffer, sz);
        free(outbuffer);
        zip_source_free(zipSrc);
    }
}
