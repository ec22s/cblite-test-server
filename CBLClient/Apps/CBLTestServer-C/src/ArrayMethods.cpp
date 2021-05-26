#include "ArrayMethods.h"
#include "MemoryMap.h"
#include "Router.h"
#include "FleeceHelpers.h"
#include "Defines.h"

#include INCLUDE_FLEECE(Fleece.h)

using namespace std;
using namespace nlohmann;
using namespace fleece;

using value_t = detail::value_t;

static void FLMutableArray_EntryDelete(void* ptr) {
    FLMutableArray_Release(static_cast<FLMutableArray>(ptr));
}


void array_methods::array_addDictionary(json& body, mg_connection* conn) {
    const auto* const dict = static_cast<FLDict>(memory_map::get(body["value"].get<string>()));
    with<FLMutableArray>(body, "array", [dict, conn](FLMutableArray a)
    {
        FLSlot slot = FLMutableArray_Append(a);
        FLSlot_SetDict(slot, dict);
        write_serialized_body(conn, reinterpret_cast<const FLValue>(a));
    });
}

void array_methods::array_addString(json& body, mg_connection* conn) {
    const auto val = body["value"].get<string>();
    with<FLMutableArray>(body, "array", [&val, conn](FLMutableArray a)
    {
        FLSlot slot = FLMutableArray_Append(a);
        FLSlot_SetString(slot, flstr(val));
        write_serialized_body(conn, reinterpret_cast<const FLValue>(a));
    });
}

void array_methods::array_create(json& body, mg_connection* conn) {
    string arrayId;
    if(body.contains("content_array")) {
        const auto content = body["content_array"];
        FLMutableArray arr = FLMutableArray_New();
        if(content.type() != value_t::array) {
            throw invalid_argument("Non-array received in array_create");
        }

        for(const auto& element : content) {
            writeFleece(arr, element);
        }

        string id = memory_map::store(arr, FLMutableArray_EntryDelete);
        mg_send_http_ok(conn, "text/plain", id.length());
        mg_write(conn, id.c_str(), id.size());
    }
}

void array_methods::array_getArray(json& body, mg_connection* conn) {
    const auto key = body["key"].get<int>();
    with<FLMutableArray>(body, "array", [key, conn](FLMutableArray a)
    {
        FLArrayIterator i;
        FLArrayIterator_Begin(a, &i);
        const FLArray subArray = FLValue_AsArray(FLArrayIterator_GetValueAt(&i, key));
        write_serialized_body(conn, reinterpret_cast<const FLValue>(subArray));
    });
}

void array_methods::array_getDictionary(json& body, mg_connection* conn) {
    const auto key = body["key"].get<int>();
    with<FLMutableArray>(body, "array", [key, conn](FLMutableArray a)
    {
        FLArrayIterator i;
        FLArrayIterator_Begin(a, &i);
        const FLDict subDict = FLValue_AsDict(FLArrayIterator_GetValueAt(&i, key));
        write_serialized_body(conn, reinterpret_cast<const FLValue>(subDict));
    });
}

void array_methods::array_getString(json& body, mg_connection* conn) {
    const auto key = body["key"].get<int>();
    with<FLMutableArray>(body, "array", [key, conn](FLMutableArray a)
    {
        FLArrayIterator i;
        FLArrayIterator_Begin(a, &i);
        const FLValue val = FLArrayIterator_GetValueAt(&i, key);
        if(FLValue_GetType(val) != kFLString) {
            throw bad_cast();
        }

        write_serialized_body(conn, val);
    });
}

void array_methods::array_setString(json& body, mg_connection* conn) {
    const auto key = body["key"].get<uint32_t>();
    const auto val = body["value"].get<string>();
    with<FLMutableArray>(body, "array", [key, &val, conn](FLMutableArray a)
    {
        FLSlot slot = FLMutableArray_Set(a, key);
        FLSlot_SetString(slot, flstr(val));
        write_serialized_body(conn, reinterpret_cast<const FLValue>(a));
    });
}
