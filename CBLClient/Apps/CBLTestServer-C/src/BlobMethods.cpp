#include "BlobMethods.h"
#include "MemoryMap.h"
#include "Defines.h"
#include "Router.h"
#include "FilePathResolver.h"

#include <cbl/CouchbaseLite.h>
#include <fstream>
using namespace std;
using namespace nlohmann;

static void CBLBlob_EntryDelete(void* ptr) {
    CBLBlob_Release(static_cast<CBLBlob *>(ptr));
}

static void FLSliceResult_EntryDelete(void* ptr) {
    auto* slice = static_cast<FLSliceResult *>(ptr);
    FLSliceResult_Release(*slice);
    free(slice);
}

namespace blob_methods {
    void blob_create(nlohmann::json& body, mg_connection* conn) {
        string contentType;
        if(body.contains("contentType")) {
            contentType = body["contentType"].get<string>();
        }

        if(body.contains("content")) {
            const auto* data = static_cast<FLSliceResult *>(memory_map::get(body["content"].get<string>()));
            auto* blob = CBLBlob_NewWithData(flstr(contentType), static_cast<FLSlice>(*data));
            write_serialized_body(conn, memory_map::store(blob, CBLBlob_EntryDelete));
        } else if(body.contains("stream")) {
            auto* stream = static_cast<CBLBlobWriteStream *>(memory_map::get(body["stream"].get<string>()));
            auto* blob = CBLBlob_NewWithStream(flstr(contentType), stream);
            write_serialized_body(conn, memory_map::store(blob, CBLBlob_EntryDelete));
        } else if(body.contains("fileURL")) {
            mg_send_http_error(conn, 501, "Not supported in C API");
        }
    }

    void blob_createImageContent(nlohmann::json& body, mg_connection* conn) {
        const auto imageLocation = file_resolution::resolve_path(body["image"].get<string>(), false);
        ifstream fin(imageLocation.c_str(), ios::binary|ios::ate);
        size_t len = fin.tellg();
        FLSliceResult* resultPtr = static_cast<FLSliceResult *>(malloc(sizeof(FLSliceResult)));
        *resultPtr = FLSliceResult_New(len);
        char* data = static_cast<char *>(malloc(len));
        fin.seekg(0, ios::beg);
        fin.read(data, len);
        fin.close();

        resultPtr->buf = data;
        write_serialized_body(conn, memory_map::store(resultPtr, FLSliceResult_EntryDelete));
    }

    void blob_createImageStream(nlohmann::json& body, mg_connection* conn) {
        mg_send_http_error(conn, 501, "Cannot use this method with passing a database");
    }

    void blob_createImageFileUrl(nlohmann::json& body, mg_connection* conn) {
        mg_send_http_error(conn, 501, "Not supported by the C API");
    }
}