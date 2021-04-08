#include "DictionaryMethods.h"
#include "MemoryMap.h"
#include "Router.h"
#include "FleeceHelpers.h"
#include "date.h"

#include <fleece/Fleece.h>
#include <cbl/CBLBlob.h>

using namespace nlohmann;
using namespace std;

static void FLMutableDict_EntryDelete(void* ptr) {
    FLMutableDict_Release(static_cast<FLMutableDict>(ptr));
}

static FLValue FLMutableDict_FindValue(FLMutableDict dict, const string& key, FLValueType type) {
    if(FLDict_Count(dict) == 0) {
        return nullptr;
    }

    FLDictIterator i;
    FLDictIterator_Begin(dict, &i);
    do {
        const auto flKey = FLDictIterator_GetKeyString(&i);
        const string foundKey(static_cast<const char*>(flKey.buf), flKey.size);
        if(foundKey == key) {
            const FLValue val = FLDictIterator_GetValue(&i);
            return (FLValue_GetType(val) == type || type == kFLUndefined) ? val : nullptr;
        }
    } while(FLDictIterator_Next(&i));

    return nullptr;
}

namespace dictionary_methods {
    void dictionary_contains(json& body, mg_connection* conn) {
        const auto key = body["key"].get<string>();
        with<FLMutableDict>(body, "dictionary", [conn, &key](FLMutableDict d)
        {
            if(FLDict_Count(d) == 0) {
                write_serialized_body(conn, false);
                return;
            }

            FLDictIterator i;
            FLDictIterator_Begin(d, &i);
            do {
                const auto flKey = FLDictIterator_GetKeyString(&i);
                const string foundKey(static_cast<const char*>(flKey.buf), flKey.size);
                if(foundKey == key) {
                    write_serialized_body(conn, true);
                    return;
                }
            } while(FLDictIterator_Next(&i));

            write_serialized_body(conn, false);
        });
    }

    void dictionary_count(json& body, mg_connection* conn) {
        with<FLMutableDict>(body, "dictionary", [conn](FLMutableDict d)
        {
            write_serialized_body(conn, FLDict_Count(d));
        });
    }

    void dictionary_create(json& body, mg_connection* conn) {
        FLMutableDict createdDict = FLMutableDict_New();
        if(body.contains("content_dict")) {
            for(const auto& [key, value] : body["content_dict"].items()) {
                writeFleece(createdDict, key, value);
            }
        }

        write_serialized_body(conn, memory_map::store(createdDict, FLMutableDict_EntryDelete));
    }

    void dictionary_getArray(json& body, mg_connection* conn) {
        const auto key = body["key"].get<string>();
        with<FLMutableDict>(body, "dictionary", [conn, &key](FLMutableDict d)
        {
            const FLValue val = FLMutableDict_FindValue(d, key, kFLArray);
            write_serialized_body(conn, val);
        });
    }

    void dictionary_getBlob(json& body, mg_connection* conn) {
        const auto key = body["key"].get<string>();
        with<FLMutableDict>(body, "dictionary", [conn, &key](FLMutableDict d)
        {
            const FLValue val = FLMutableDict_FindValue(d, key, kFLDict);
            if(!val) {
                write_serialized_body(conn, nullptr);
            } else {
                write_serialized_body(conn, FLDict_IsBlob(FLValue_AsDict(val)) ? val : nullptr);
            }
        });
    }

    void dictionary_getBoolean(json& body, mg_connection* conn) {
        const auto key = body["key"].get<string>();
        with<FLMutableDict>(body, "dictionary", [conn, &key](FLMutableDict d)
        {
            const FLValue val = FLMutableDict_FindValue(d, key, kFLBoolean);
            if(!val) {
                write_serialized_body(conn, false);
            } else {
                write_serialized_body(conn, val);
            }
        });
    }

    void dictionary_getDate(json& body, mg_connection* conn) {
        const auto key = body["key"].get<string>();
        with<FLMutableDict>(body, "dictionary", [conn, &key](FLMutableDict d)
        {
            const FLValue val = FLMutableDict_FindValue(d, key, kFLUndefined);
            if(!val) {
                write_serialized_body(conn, nullptr);
                return;
            }

            const FLTimestamp timestamp = FLValue_AsTimestamp(val);
            if(timestamp == INT64_MIN) {
                write_serialized_body(conn, nullptr);
                return;
            }

            if(FLValue_GetType(val) == kFLString) {
                write_serialized_body(conn, val);
                return;
            }

            char dateString[kFormattedISO8601DateMaxSize];
            FormatISO8601Date(dateString, timestamp, true);
            write_serialized_body(conn, dateString);
        });
    }

    void dictionary_getDictionary(json& body, mg_connection* conn) {
        const auto key = body["key"].get<string>();
        with<FLMutableDict>(body, "dictionary", [conn, &key](FLMutableDict d)
        {
            const FLValue val = FLMutableDict_FindValue(d, key, kFLDict);
            if(FLDict_IsBlob(FLValue_AsDict(val))) {
                write_serialized_body(conn, nullptr);
            } else {
                write_serialized_body(conn, val);
            }
        });
    }

    void dictionary_getDouble(json& body, mg_connection* conn) {
        const auto key = body["key"].get<string>();
        with<FLMutableDict>(body, "dictionary", [conn, &key](FLMutableDict d)
        {
            const FLValue val = FLMutableDict_FindValue(d, key, kFLNumber);
            if(!val) {
                write_serialized_body(conn, 0.0);
            } else {
                write_serialized_body(conn, FLValue_AsDouble(val));
            }
        });
    }

    void dictionary_getFloat(json& body, mg_connection* conn) {
        const auto key = body["key"].get<string>();
        with<FLMutableDict>(body, "dictionary", [conn, &key](FLMutableDict d)
        {
            const FLValue val = FLMutableDict_FindValue(d, key, kFLNumber);
            if(!val) {
                write_serialized_body(conn, 0.0f);
            } else {
                write_serialized_body(conn, FLValue_AsFloat(val));
            }
        });
    }

    void dictionary_getInt(json& body, mg_connection* conn) {
        const auto key = body["key"].get<string>();
        with<FLMutableDict>(body, "dictionary", [conn, &key](FLMutableDict d)
        {
            const FLValue val = FLMutableDict_FindValue(d, key, kFLNumber);
            if(!val) {
                write_serialized_body(conn, 0);
            } else {
                auto intVal = static_cast<int32_t>(FLValue_AsInt(val));
                write_serialized_body(conn, intVal);
            }
        });
    }

    void dictionary_getKeys(json& body, mg_connection* conn) {
        with<FLMutableDict>(body, "dictionary", [conn](FLMutableDict d)
        {
            json keys(0, nullptr);
            if(FLDict_Count(d) == 0) {
                write_serialized_body(conn, keys);
                return;
            }

            FLDictIterator i;
            FLDictIterator_Begin(d, &i);
            do {
                const auto flKey = FLDictIterator_GetKeyString(&i);
                const string key(static_cast<const char*>(flKey.buf), flKey.size);
                keys.push_back(key);
            } while(FLDictIterator_Next(&i));

            write_serialized_body(conn, keys);
        });
    }

    void dictionary_getLong(json& body, mg_connection* conn) {
        const auto key = body["key"].get<string>();
        with<FLMutableDict>(body, "dictionary", [conn, &key](FLMutableDict d)
        {
            const FLValue val = FLMutableDict_FindValue(d, key, kFLNumber);
            if(!val) {
                write_serialized_body(conn, 0LL);
            } else {
                write_serialized_body(conn, FLValue_AsInt(val));
            }
        });
    }

    void dictionary_getString(json& body, mg_connection* conn) {
        const auto key = body["key"].get<string>();
        with<FLMutableDict>(body, "dictionary", [conn, &key](FLMutableDict d)
        {
            const FLValue val = FLMutableDict_FindValue(d, key, kFLString);
            write_serialized_body(conn, val);
        });
    }

    void dictionary_getValue(json& body, mg_connection* conn) {
        const auto key = body["key"].get<string>();
        with<FLMutableDict>(body, "dictionary", [conn, &key](FLMutableDict d)
        {
            const FLValue val = FLMutableDict_FindValue(d, key, kFLUndefined);
            write_serialized_body(conn, val);
        });
    }

    void dictionary_remove(json& body, mg_connection* conn) {
        const auto key = body["key"].get<string>();
        with<FLMutableDict>(body, "dictionary", [conn, &key](FLMutableDict d)
        {
            const FLString flKey = { key.data(), key.size() };
            FLMutableDict_Remove(d, flKey);
            write_serialized_body(conn, reinterpret_cast<FLValue>(d));
        });
    }

    void dictionary_setArray(json& body, mg_connection* conn) {
        const auto key = body["key"].get<string>();
        const auto val = body["value"];
        with<FLMutableDict>(body, "dictionary", [conn, &key, &val](FLMutableDict d)
        {
            FLString flKey { key.data(), key.size() };
            FLSlot slot = FLMutableDict_Set(d, flKey);

            FLMutableArray subArr = FLMutableArray_New();
            writeFleece(subArr, val);
            FLSlot_SetMutableArray(slot, subArr);
            FLMutableArray_Release(subArr);
            write_serialized_body(conn, reinterpret_cast<FLValue>(d));
        });
    }

    void dictionary_setBlob(json& body, mg_connection* conn) {
        const auto key = body["key"].get<string>();
        auto* val = static_cast<CBLBlob *>(memory_map::get(body["value"].get<string>()));
        with<FLMutableDict>(body, "dictionary", [conn, &key, val](FLMutableDict d)
        {
            FLString flKey { key.data(), key.size() };
            FLSlot slot = FLMutableDict_Set(d, flKey);
            FLSlot_SetBlob(slot, val);
            write_serialized_body(conn, reinterpret_cast<FLValue>(d));
        });
    }

    void dictionary_setBoolean(json& body, mg_connection* conn) {
        const auto key = body["key"].get<string>();
        const auto val = body["value"].get<bool>();
        with<FLMutableDict>(body, "dictionary", [conn, &key, val](FLMutableDict d)
        {
            const FLString flKey = { key.data(), key.size() };
            FLSlot slot = FLMutableDict_Set(d, flKey);
            FLSlot_SetBool(slot, val);
            write_serialized_body(conn, reinterpret_cast<FLValue>(d));
        });
    }

    void dictionary_setDate(json& body, mg_connection* conn) {
        dictionary_setString(body, conn);
    }

    void dictionary_setDictionary(json& body, mg_connection* conn) {
        const auto key = body["key"].get<string>();
        const auto val = body["value"];
        with<FLMutableDict>(body, "dictionary", [conn, &key, &val](FLMutableDict d)
        {
            FLString flKey { key.data(), key.size() };
            FLSlot slot = FLMutableDict_Set(d, flKey);

            FLMutableDict subDict = FLMutableDict_New();
            for(const auto& [key, value] : val.items()) {
                writeFleece(subDict, key, value);
            }

            FLSlot_SetMutableDict(slot, subDict);
            FLMutableDict_Release(subDict);
            write_serialized_body(conn, reinterpret_cast<FLValue>(d));
        });
    }

    void dictionary_setDouble(json& body, mg_connection* conn) {
        const auto key = body["key"].get<string>();
        const auto val = body["value"].get<double>();
        with<FLMutableDict>(body, "dictionary", [conn, &key, val](FLMutableDict d)
        {
            const FLString flKey = { key.data(), key.size() };
            FLSlot slot = FLMutableDict_Set(d, flKey);
            FLSlot_SetDouble(slot, val);
            write_serialized_body(conn, reinterpret_cast<FLValue>(d));
        });
    }

    void dictionary_setFloat(json& body, mg_connection* conn) {
        const auto key = body["key"].get<string>();
        const auto val = body["value"].get<float>();
        with<FLMutableDict>(body, "dictionary", [conn, &key, val](FLMutableDict d)
        {
            const FLString flKey = { key.data(), key.size() };
            FLSlot slot = FLMutableDict_Set(d, flKey);
            FLSlot_SetFloat(slot, val);
            write_serialized_body(conn, reinterpret_cast<FLValue>(d));
        });
    }

    void dictionary_setInt(json& body, mg_connection* conn) {
        const auto key = body["key"].get<string>();
        const auto val = body["value"].get<int>();
        with<FLMutableDict>(body, "dictionary", [conn, &key, val](FLMutableDict d)
        {
            const FLString flKey = { key.data(), key.size() };
            FLSlot slot = FLMutableDict_Set(d, flKey);
            FLSlot_SetInt(slot, val);
            write_serialized_body(conn, reinterpret_cast<FLValue>(d));
        });
    }

    void dictionary_setLong(json& body, mg_connection* conn) {
        const auto key = body["key"].get<string>();
        const auto val = body["value"].get<int64_t>();
        with<FLMutableDict>(body, "dictionary", [conn, &key, val](FLMutableDict d)
        {
            const FLString flKey = { key.data(), key.size() };
            FLSlot slot = FLMutableDict_Set(d, flKey);
            FLSlot_SetInt(slot, val);
            write_serialized_body(conn, reinterpret_cast<FLValue>(d));
        });
    }

    void dictionary_setString(json& body, mg_connection* conn) {
        const auto key = body["key"].get<string>();
        const auto val = body["value"].get<string>();
        with<FLMutableDict>(body, "dictionary", [conn, &key, &val](FLMutableDict d)
        {
            const FLString flKey = { key.data(), key.size() };
            FLSlot slot = FLMutableDict_Set(d, flKey);
            FLSlot_SetString(slot, {val.data(), val.size() });
            write_serialized_body(conn, reinterpret_cast<FLValue>(d));
        });
    }

    void dictionary_setValue(json& body, mg_connection* conn) {
        const auto key = body["key"].get<string>();
        const auto val = body["value"];
        with<FLMutableDict>(body, "dictionary", [conn, &key, val](FLMutableDict d)
        {
            writeFleece(d, key, val);
            write_serialized_body(conn, reinterpret_cast<FLValue>(d));
        });
    }
    void dictionary_toMap(json& body, mg_connection* conn) {
        with<FLMutableDict>(body, "dictionary", [conn](FLMutableDict d)
        {
            write_serialized_body(conn, reinterpret_cast<FLValue>(d));
        });
    }

    void dictionary_toMutableDictionary(json& body, mg_connection* conn) {
        // WEIRD: This doesn't do anything different than dictionary_create
        FLMutableDict createdDict = FLMutableDict_New();
        if(body.contains("dictionary")) {
            for(const auto& [key, value] : body.items()) {
                writeFleece(createdDict, key, value);
            }
        }

        write_serialized_body(conn, memory_map::store(createdDict, FLMutableDict_EntryDelete));
    }
}