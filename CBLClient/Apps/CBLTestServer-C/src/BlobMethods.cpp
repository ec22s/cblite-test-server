#include "BlobMethods.h"
#include "MemoryMap.h"
#include "Defines.h"
#include "Router.h"
#include "FilePathResolver.h"

#include INCLUDE_CBL(CouchbaseLite.h)
#include <fstream>
using namespace std;
using namespace nlohmann;

static void CBLBlob_EntryDelete(void* ptr) {
    CBLBlob_Release(static_cast<CBLBlob *>(ptr));
}

static void CBLBlobWriteStream_EntryDelete(void* ptr) {
    CBLBlobWriter_Close(static_cast<CBLBlobWriteStream *>(ptr));
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
            auto* blob = CBLBlob_CreateWithData(flstr(contentType), static_cast<FLSlice>(*data));
            write_serialized_body(conn, memory_map::store(blob, CBLBlob_EntryDelete));
        } else if(body.contains("stream")) {
            auto* stream = static_cast<CBLBlobWriteStream *>(memory_map::get(body["stream"].get<string>()));
            auto* blob = CBLBlob_CreateWithStream(flstr(contentType), stream);
            write_serialized_body(conn, memory_map::store(blob, CBLBlob_EntryDelete));
        } else if(body.contains("fileURL")) {
            mg_send_http_error(conn, 501, "Not supported in C API");
        }
    }

    void blob_createImageContent(nlohmann::json& body, mg_connection* conn) {
        const auto imageLocation = file_resolution::resolve_path(body["image"].get<string>(), false);
        ifstream fin(imageLocation.c_str(), ios::binary|ios::ate);
        fin.exceptions(ios::badbit|ios::failbit);
        size_t len = fin.tellg();
        FLSliceResult* resultPtr = static_cast<FLSliceResult *>(malloc(sizeof(FLSliceResult)));
        auto data = static_cast<char *>(malloc(len));
        fin.seekg(0, ios::beg);
        fin.read(const_cast<char *>(data), len);
        fin.close();

        *resultPtr = FLSliceResult_CreateWith(data, len);
        write_serialized_body(conn, memory_map::store(resultPtr, FLSliceResult_EntryDelete));
    }

    void blob_createImageStream(nlohmann::json& body, mg_connection* conn) {
        string imageLocation = file_resolution::resolve_path(body["image"].get<string>(), false);
        with<CBLDatabase *>(body, "database", [&imageLocation, conn](CBLDatabase* db) {
            CBLBlobWriteStream* stream;
            CBLError err;
            TRY(stream = CBLBlobWriter_Create(db, &err), err);
            ifstream fin(imageLocation, ios_base::binary);
            if(!fin.good()) {
                throw domain_error(string("Unable to open ") + imageLocation);
            }

            char buffer[8192];
            streamsize read;
            while((read = fin.readsome(buffer, 8192)) > 0) {
                if(!CBLBlobWriter_Write(stream, buffer, read, &err)) {
                    CBLBlobWriter_Close(stream);
                    fin.close();
                    string errMsg = to_string(CBLError_Message(&err));
                    throw domain_error(errMsg);
                }
            }

            fin.close();
            write_serialized_body(conn, memory_map::store(stream, CBLBlobWriteStream_EntryDelete));
        });
    }

    void blob_createImageFileUrl(nlohmann::json& body, mg_connection* conn) {
        mg_send_http_error(conn, 501, "Not supported by the C API");
    }
}
