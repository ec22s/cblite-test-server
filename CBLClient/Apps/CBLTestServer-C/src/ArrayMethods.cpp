#include "ArrayMethods.h"
#include "MemoryMap.h"
#include "Router.h"

#include "fleece/Fleece.h"
#include <string>
using namespace std;
using namespace nlohmann;
using namespace fleece;

using value_t = detail::value_t;

static void writeFleece(FLMutableDict dict, const string& key, const json& element);
static void writeFleece(FLMutableArray array, const json& element);

static void addElement(FLSlot slot, const json& element) {
    switch(element.type()) {
        case value_t::number_float:
            FLSlot_SetDouble(slot, element.get<double>());
            break;
        case value_t::number_integer:
            FLSlot_SetInt(slot, element.get<int64_t>());
            break;
        case value_t::number_unsigned:
            FLSlot_SetUInt(slot, element.get<uint64_t>());
            break;
        case value_t::string:
        {
            auto s = element.get<string>();
            FLString val = { s.data(), s.size() };
            FLSlot_SetString(slot, val);
            break;
        }
        case value_t::boolean:
            FLSlot_SetBool(slot, element.get<bool>());
            break;
        case value_t::null:
            FLSlot_SetNull(slot);
            break;
        case value_t::array:
        {
            FLMutableArray nextArr = FLMutableArray_New();
            for(const auto& subelement : element) {
                writeFleece(nextArr, subelement);
            }

            FLSlot_SetMutableArray(slot, nextArr);
            FLMutableArray_Release(nextArr);
            break;
        }
        case value_t::object:
        {
            FLMutableDict nextDict = FLMutableDict_New();
            for(auto& [key, value] : element.items()) {
                writeFleece(nextDict, key, value);
            }

            FLSlot_SetMutableDict(slot, nextDict);
            FLMutableDict_Release(nextDict);
            break;
        }
        case value_t::binary:
        case value_t::discarded:
        default:
            throw domain_error("Invalid JSON entry in writeFleece!");
    }
}

static void writeFleece(FLMutableArray array, const json& element) {
    FLSlot slot = FLMutableArray_Append(array);
    addElement(slot, element);
}

static void writeFleece(FLMutableDict dict, const string& key, const json& element) {
    FLString keySlice = { key.data(), key.size() };
    FLSlot slot = FLMutableDict_Set(dict, keySlice);
    addElement(slot, element);
}

void array_methods::array_addDictionary(json& body, mg_connection* conn) {
    const auto* const dict = static_cast<FLDict>(memory_map::get(body["value"].get<string>()));
    with<FLMutableArray>(body, "array", [dict, conn](FLMutableArray a)
    {
        const FLSlot slot = FLMutableArray_Append(a);
        FLSlot_SetDict(slot, dict);
        write_serialized_body(conn, reinterpret_cast<const FLValue>(a), true);
    });
}

void array_methods::array_addString(json& body, mg_connection* conn) {
    const auto val = body["value"].get<string>();
    with<FLMutableArray>(body, "array", [&val, conn](FLMutableArray a)
    {
        const FLString flStr = {val.data(), val.size() };
        const FLSlot slot = FLMutableArray_Append(a);
        FLSlot_SetString(slot, flStr);
        write_serialized_body(conn, reinterpret_cast<const FLValue>(a), true);
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

        string id = memory_map::store(arr, [](void* p) { FLMutableArray_Release(static_cast<FLMutableArray>(p)); });
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
        write_serialized_body(conn, reinterpret_cast<const FLValue>(subArray), true);
    });
}

void array_methods::array_getDictionary(json& body, mg_connection* conn) {
    const auto key = body["key"].get<int>();
    with<FLMutableArray>(body, "array", [key, conn](FLMutableArray a)
    {
        FLArrayIterator i;
        FLArrayIterator_Begin(a, &i);
        const FLDict subDict = FLValue_AsDict(FLArrayIterator_GetValueAt(&i, key));
        write_serialized_body(conn, reinterpret_cast<const FLValue>(subDict), true);
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

        write_serialized_body(conn, val, true);
    });
}
