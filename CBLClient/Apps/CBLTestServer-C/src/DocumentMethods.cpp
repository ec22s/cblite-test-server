#include "DocumentMethods.h"
#include "Router.h"
#include "MemoryMap.h"
#include "FleeceHelpers.h"
#include "Defines.h"
#include "date.h"

#include INCLUDE_CBL(CouchbaseLite.h)
using namespace std;
using namespace nlohmann;
using namespace chrono;
using namespace date;

void CBLDocument_EntryDelete(void* ptr) {
    CBLDocument_Release(static_cast<CBLDocument *>(ptr));
}

void DateTime_EntryDelete(void* ptr) {
    delete (milliseconds *)ptr;
}

namespace document_methods {
    void document_create(json& body, mg_connection* conn) {
        CBLDocument* doc;
        if(body.contains("id")) {
            const auto id = body["id"].get<string>();
            doc = CBLDocument_CreateWithID({id.data(), id.size() });
        } else {
            doc = CBLDocument_Create();
        }

        if(body.contains("dictionary")) {
            FLMutableDict newContent = FLMutableDict_New();
            for(auto& [ key, value ] : body["dictionary"].items()) {
                writeFleece(newContent, key, value);
            }

            CBLDocument_SetProperties(doc, newContent);
            FLMutableDict_Release(newContent);
        }

        write_serialized_body(conn, memory_map::store(doc, CBLDocument_EntryDelete));
    }

    void document_delete(json& body, mg_connection* conn) {
        with<CBLDatabase *>(body, "database", [conn, &body](CBLDatabase *db)
        {
            with<const CBLDocument*>(body, "document", [db](const CBLDocument* doc)
            {
                CBLError err;
                TRY(CBLDatabase_DeleteDocument(db, doc, &err), err)
            });            

            mg_send_http_ok(conn, "text/plain", 0);
        });
    }

    void document_getId(json& body, mg_connection* conn) {
        with<const CBLDocument*>(body, "document", [conn](const CBLDocument* doc)
        {
            auto flID = CBLDocument_ID(doc);
            string id(static_cast<const char *>(flID.buf), flID.size);
            write_serialized_body(conn, id);
        });
    }

    void document_getString(json& body, mg_connection* conn) {
        const auto key = body["key"].get<string>();
        with<const CBLDocument*>(body, "document", [&key, conn](const CBLDocument* doc)
        {
            const auto* flVal = FLDict_FindValue(CBLDocument_Properties(doc), key, kFLString);
            write_serialized_body(conn, flVal);
        });
    }

    void document_setString(json& body, mg_connection* conn) {
        const auto key = body["key"].get<string>();
        const auto value = body["value"].get<string>();
        const auto handle = body["document"].get<string>();
        with<CBLDocument*>(body, "document", [&key, &value](CBLDocument* doc)
        {
            FLMutableDict properties = CBLDocument_MutableProperties(doc);
            FLSlot slot = FLMutableDict_Set(properties, { key.data(), key.size() });
            FLSlot_SetString(slot, { value.data(), value.size() });
        });

        write_serialized_body(conn, handle);
    }

    void document_count(json& body, mg_connection* conn) {
        with<const CBLDocument*>(body, "document", [conn](const CBLDocument* doc)
        {
            write_serialized_body(conn, FLDict_Count(CBLDocument_Properties(doc)));
        });
    }

    void document_getInt(json& body, mg_connection* conn) {
        const auto key = body["key"].get<string>();
        with<const CBLDocument*>(body, "document", [&key, conn](const CBLDocument* doc)
        {
            const auto* flVal = FLDict_FindValue(CBLDocument_Properties(doc), key, kFLNumber);
            if(!flVal) {
                write_serialized_body(conn, 0);
            } else {
                write_serialized_body(conn, static_cast<int32_t>(FLValue_AsInt(flVal)));
            }
        });
    }

    void document_setInt(json& body, mg_connection* conn) {
        const auto key = body["key"].get<string>();
        const auto value = body["value"].get<int64_t>();
        const auto handle = body["document"].get<string>();
        with<CBLDocument*>(body, "document", [&key, &value](CBLDocument* doc)
        {
            FLMutableDict properties = CBLDocument_MutableProperties(doc);
            FLSlot slot = FLMutableDict_Set(properties, { key.data(), key.size() });
            FLSlot_SetInt(slot, value);
        });

        write_serialized_body(conn, handle);
    }

    void document_getLong(json& body, mg_connection* conn) {
        const auto key = body["key"].get<string>();
        with<const CBLDocument*>(body, "document", [&key, conn](const CBLDocument* doc)
        {
            const auto* flVal = FLDict_FindValue(CBLDocument_Properties(doc), key, kFLNumber);
            if(!flVal) {
                write_serialized_body(conn, 0LL);
            } else {
                write_serialized_body(conn, FLValue_AsInt(flVal));
            }
        });
    }

    void document_setLong(json& body, mg_connection* conn) {
        document_setInt(body, conn);
    }

    void document_getFloat(json& body, mg_connection* conn) {
        const auto key = body["key"].get<string>();
        with<const CBLDocument*>(body, "document", [&key, conn](const CBLDocument* doc)
        {
            const auto* flVal = FLDict_FindValue(CBLDocument_Properties(doc), key, kFLNumber);
            if(!flVal) {
                write_serialized_body(conn, 0.0f);
            } else {
                write_serialized_body(conn, FLValue_AsFloat(flVal));
            }
        });
    }

    void document_setFloat(json& body, mg_connection* conn) {
        const auto key = body["key"].get<string>();
        const auto value = body["value"].get<float>();
        const auto handle = body["document"].get<string>();
        with<CBLDocument*>(body, "document", [&key, &value](CBLDocument* doc)
        {
            FLMutableDict properties = CBLDocument_MutableProperties(doc);
            FLSlot slot = FLMutableDict_Set(properties, { key.data(), key.size() });
            FLSlot_SetFloat(slot, value);
        });

        write_serialized_body(conn, handle);
    }

    void document_getDouble(json& body, mg_connection* conn) {
        const auto key = body["key"].get<string>();
        with<const CBLDocument*>(body, "document", [&key, conn](const CBLDocument* doc)
        {
            const auto* flVal = FLDict_FindValue(CBLDocument_Properties(doc), key, kFLNumber);
            if(!flVal) {
                write_serialized_body(conn, 0.0);
            } else {
                write_serialized_body(conn, FLValue_AsDouble(flVal));
            }
        });
    }

    void document_setDouble(json& body, mg_connection* conn) {
        const auto key = body["key"].get<string>();
        const auto valStr = body["value"].get<string>();
        double value = atof(valStr.c_str());
        const auto handle = body["document"].get<string>();
        with<CBLDocument*>(body, "document", [&key, &value](CBLDocument* doc)
        {
            FLMutableDict properties = CBLDocument_MutableProperties(doc);
            FLSlot slot = FLMutableDict_Set(properties, { key.data(), key.size() });
            FLSlot_SetDouble(slot, value);
        });

        write_serialized_body(conn, handle);
    }

    void document_getBoolean(json& body, mg_connection* conn) {
        const auto key = body["key"].get<string>();
        with<const CBLDocument*>(body, "document", [&key, conn](const CBLDocument* doc)
        {
            const auto* flVal = FLDict_FindValue(CBLDocument_Properties(doc), key, kFLBoolean);
            if(!flVal) {
                write_serialized_body(conn, false);
            } else {
                write_serialized_body(conn, FLValue_AsBool(flVal));
            }
        });
    }

    void document_setBoolean(json& body, mg_connection* conn) {
        const auto key = body["key"].get<string>();
        const auto value = body["value"].get<bool>();
        const auto handle = body["document"].get<string>();
        with<CBLDocument*>(body, "document", [&key, &value](CBLDocument* doc)
        {
            FLMutableDict properties = CBLDocument_MutableProperties(doc);
            FLSlot slot = FLMutableDict_Set(properties, { key.data(), key.size() });
            FLSlot_SetBool(slot, value);
        });

        write_serialized_body(conn, handle);
    }

    void document_getBlob(json& body, mg_connection* conn) {
        const auto key = body["key"].get<string>();
        with<const CBLDocument*>(body, "document", [&key, conn](const CBLDocument* doc)
        {
            const auto* flVal = FLDict_FindValue(CBLDocument_Properties(doc), key, kFLDict);
            if(!flVal || !FLDict_IsBlob(reinterpret_cast<FLDict>(flVal))) {
                write_serialized_body(conn, nullptr);
            } else {
                write_serialized_body(conn, flVal);
            }
        });
    }

    void document_setBlob(json& body, mg_connection* conn) {
        const auto key = body["key"].get<string>();
        auto* value = static_cast<CBLBlob *>(memory_map::get(body["value"].get<string>()));
        const auto handle = body["document"].get<string>();
        with<CBLDocument*>(body, "document", [&key, &value](CBLDocument* doc)
        {
            FLMutableDict properties = CBLDocument_MutableProperties(doc);
            FLSlot slot = FLMutableDict_Set(properties, { key.data(), key.size() });
            FLSlot_SetBlob(slot, value);
        });

        write_serialized_body(conn, handle);
    }

    void document_getArray(json& body, mg_connection* conn) {
        const auto key = body["key"].get<string>();
        with<const CBLDocument*>(body, "document", [&key, conn](const CBLDocument* doc)
        {
            const auto* flVal = FLDict_FindValue(CBLDocument_Properties(doc), key, kFLArray);
            write_serialized_body(conn, flVal);
        });
    }

    void document_setArray(json& body, mg_connection* conn) {
        const auto key = body["key"].get<string>();
        const auto value = body["value"];
        const auto handle = body["document"].get<string>();
        with<CBLDocument*>(body, "document", [&body, &key, &value](CBLDocument* doc)
        {
            with<FLArray>(body, "value", [&key, doc](FLArray arr)
            {
                FLString flKey { key.data(), key.size() };
                FLMutableDict properties = CBLDocument_MutableProperties(doc);
                FLSlot slot = FLMutableDict_Set(properties, flKey);
                FLSlot_SetArray(slot, arr);
            });
        });

        write_serialized_body(conn, handle);
    }

    void document_getDate(json& body, mg_connection* conn) {
        const auto key = body["key"].get<string>();
        with<const CBLDocument*>(body, "document", [conn, &key](const CBLDocument* doc)
        {
            FLDict properties = CBLDocument_Properties(doc);
            const FLValue val = FLDict_FindValue(properties, key, kFLUndefined);
            if(!val) {
                write_serialized_body(conn, nullptr);
                return;
            }

            const FLTimestamp timestamp = FLValue_AsTimestamp(val);
            if(timestamp == INT64_MIN) {
                write_serialized_body(conn, nullptr);
                return;
            }

            auto* stored = new milliseconds(timestamp);
            write_serialized_body(conn, memory_map::store(stored, DateTime_EntryDelete));
        });
    }

    void document_setDate(json& body, mg_connection* conn) {
        const auto key = body["key"].get<string>();
        auto* value = (chrono::milliseconds *)memory_map::get(body["value"].get<string>());
        const auto handle = body["document"].get<string>();
        with<CBLDocument*>(body, "document", [conn, &key, value](CBLDocument* doc)
        {
            FLMutableDict properties = CBLDocument_MutableProperties(doc);
            FLString flKey { key.data(), key.size() };
            FLMutableDict_SetInt(properties, flKey, value->count());
        });

        write_serialized_body(conn, handle);
    }

    void document_setData(json& body, mg_connection* conn) {
        const auto value = body["data"];
        const auto handle = body["document"].get<string>();
        with<CBLDocument*>(body, "document", [&value](CBLDocument* doc)
        {
            FLMutableDict properties = FLMutableDict_New();
            for(const auto& [ key, val] : value.items()) {
                writeFleece(properties, key, val);
            }

            CBLDocument_SetProperties(doc, properties);
            FLMutableDict_Release(properties);
        });

        write_serialized_body(conn, handle);
    }

    void document_getDictionary(json& body, mg_connection* conn) {
        const auto key = body["key"].get<string>();
        with<const CBLDocument*>(body, "document", [&key, conn](const CBLDocument* doc)
        {
            const auto* flVal = FLDict_FindValue(CBLDocument_Properties(doc), key, kFLDict);
            if(flVal && FLDict_IsBlob(reinterpret_cast<FLDict>(flVal))) {
                write_serialized_body(conn, nullptr);
            } else {
                auto pointer = memory_map::store(flVal, nullptr);
                write_serialized_body(conn, pointer);
            }
        });
    }

    void document_setDictionary(json& body, mg_connection* conn) {
        const auto key = body["key"].get<string>();
        const auto value = body["value"];
        const auto handle = body["document"].get<string>();
        with<CBLDocument*>(body, "document", [&body, &key, &value](CBLDocument* doc)
        {
            with<FLDict>(body, "value", [&key, doc](FLDict dict)
            {
                FLString flKey { key.data(), key.size() };
                FLMutableDict properties = CBLDocument_MutableProperties(doc);
                FLSlot slot = FLMutableDict_Set(properties, flKey);
                FLSlot_SetDict(slot, dict);
            });
        });

        write_serialized_body(conn, handle);
    }

    void document_getKeys(json& body, mg_connection* conn) {
        with<const CBLDocument*>(body, "document", [conn](const CBLDocument* doc)
        {
            json keys(0, nullptr);
            FLDict properties = CBLDocument_Properties(doc);
            if(FLDict_Count(properties) == 0) {
                write_serialized_body(conn, keys);
                return;
            }

            FLDictIterator i;
            FLDictIterator_Begin(properties, &i);
            do {
                const auto flKey = FLDictIterator_GetKeyString(&i);
                const string key(static_cast<const char*>(flKey.buf), flKey.size);
                keys.push_back(key);
            } while(FLDictIterator_Next(&i));

            write_serialized_body(conn, keys);
        });
    }

    void document_toMap(json& body, mg_connection* conn) {
        with<const CBLDocument*>(body, "document", [conn](const CBLDocument* doc)
        {
           write_serialized_body(conn, reinterpret_cast<FLValue>(CBLDocument_Properties(doc))); 
        });
    }

    void document_toMutable(json& body, mg_connection* conn) {
        with<const CBLDocument*>(body, "document", [conn](const CBLDocument* doc)
        {
            CBLDocument* copy = CBLDocument_MutableCopy(doc);
            write_serialized_body(conn, memory_map::store(copy, CBLDocument_EntryDelete));
        });
    }

    void document_removeKey(json& body, mg_connection* conn) {
        const auto key = body["key"].get<string>();
        const auto handle = body["document"].get<string>();
        with<CBLDocument*>(body, "document", [&key](CBLDocument* doc)
        {
            FLMutableDict properties = CBLDocument_MutableProperties(doc);
            FLMutableDict_Remove(properties, { key.data(), key.size() });
        });

        write_serialized_body(conn, handle);
    }

    void document_contains(json& body, mg_connection* conn) {
        const auto key = body["key"].get<string>();
        with<const CBLDocument*>(body, "document", [&key, conn](const CBLDocument* doc)
        {
            FLDict properties = CBLDocument_Properties(doc);
            bool exists = FLDict_Get(properties, { key.data(), key.size() }) != nullptr;
            write_serialized_body(conn, exists);
        });
    }

    void document_getValue(json& body, mg_connection* conn) {
        const auto key = body["key"].get<string>();
        with<const CBLDocument*>(body, "document", [&key, conn](const CBLDocument* doc)
        {
            const auto* flVal = FLDict_FindValue(CBLDocument_Properties(doc), key, kFLUndefined);
            write_serialized_body(conn, flVal);
        });
    }

    void document_setValue(json& body, mg_connection* conn) {
        const auto key = body["key"].get<string>();
        const auto value = body["value"];
        const auto handle = body["document"].get<string>();
        with<CBLDocument*>(body, "document", [&key, &value](CBLDocument* doc)
        {
            FLMutableDict properties = CBLDocument_MutableProperties(doc);
            writeFleece(properties, key, value);
        });

        write_serialized_body(conn, handle);
    }
}
