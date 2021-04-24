#include "ValueSerializer.h"
#include <sstream>

using namespace std;
using namespace nlohmann;
using value_t = detail::value_t;

namespace value_serializer {
    string serialize(nullptr_t n) {
        return "null";
    }

    string serialize(bool b) {
        return b ? "true" : "false";
    }

    string serialize(const string &s) {
        string retVal;
        retVal.reserve(s.size() + 2);
        retVal += "\"";
        retVal += s;
        retVal += "\"";
        return retVal;
    }

    string serialize(const json& j) {
        switch(j.type()) {
            case value_t::null:
                return serialize(nullptr);
            case value_t::boolean:
                return serialize(j.get<bool>());
            case value_t::number_integer:
                {
                    const auto val = j.get<int64_t>();
                    if(val >= numeric_limits<int32_t>::max() || val <= numeric_limits<int32_t>::min()) {
                        return serialize(val);
                    }

                    return serialize(static_cast<int32_t>(val));
                }
            case value_t::number_unsigned:
                {
                    const auto val = j.get<uint64_t>();
                    if(val <= numeric_limits<uint32_t>::min()) {
                        return serialize(val);
                    }

                    return serialize(static_cast<uint32_t>(val));
                }
            case value_t::number_float:
                return serialize(j.get<double>());
            case value_t::string:
                return serialize(j.get<string>());
            case value_t::array:
                {
                    json newJson;
                    for(const auto& element : j) {
                        newJson.push_back(serialize(element));
                    }

                    return newJson.dump();
                }
            case value_t::object:
                {
                    json newJson;
                    for(auto& [key, value] : j.items()) {
                        newJson[key] = serialize(value);
                    }

                    return newJson.dump();
                }
            default:
            case value_t::binary:
            case value_t::discarded:
                break;
        }

        throw runtime_error("Invalid item in JSON!");
    }

    string serialize(const FLValue val) {
        if(!val) {
            return serialize(nullptr);
        }

        switch(FLValue_GetType(val)) {
            case kFLBoolean:
                return serialize(FLValue_AsBool(val));
            case kFLNull:
                return serialize(nullptr);
            case kFLNumber:
                if(FLValue_IsInteger(val)) {
                    if(FLValue_IsUnsigned(val)) {
                        return serialize(FLValue_AsUnsigned(val));
                    }

                    return serialize(FLValue_AsInt(val));
                }

                return FLValue_IsDouble(val) ? serialize(FLValue_AsDouble(val)) : serialize(FLValue_AsFloat(val));
            case kFLString:
            {
                const FLString flStr = FLValue_AsString(val);
                const string str(static_cast<const char*>(flStr.buf), flStr.size);
                return serialize(str);
            }
            case kFLArray:
            {
                json stringList;
                FLArrayIterator i;
                FLArrayIterator_Begin(FLValue_AsArray(val), &i);
                const int count = static_cast<int>(FLArrayIterator_GetCount(&i));
                for(int idx = 0; idx < count; idx++) {
                    stringList.push_back(serialize(FLArrayIterator_GetValueAt(&i, idx)));
                }

                return stringList.dump();
            }
            case kFLDict:
            {
                json stringMap;
                FLDictIterator i;
                FLDictIterator_Begin(FLValue_AsDict(val), &i);
                const int count = static_cast<int>(FLDictIterator_GetCount(&i));
                for(int idx = 0; idx < count; FLDictIterator_Next(&i), idx++) {
                    const FLString keyFLStr = FLDictIterator_GetKeyString(&i);
                    string keyStr(static_cast<const char *>(keyFLStr.buf), keyFLStr.size);
                    stringMap[keyStr] = serialize(FLDictIterator_GetValue(&i));
                }

                return stringMap.dump();
            }
            case kFLUndefined:
            case kFLData:
            default:
                break;
        }

        throw domain_error("Invalid type in serialize");
    }


    static json deserialize_inner(const json& j) {
        const auto& str = j.get<string>();
        if(str == "null") {
            return json(nullptr);
        }

        if(str == "true") {
            return json(true);
        }

        if(str == "false") {
            return json(false);
        }

        if(str[0] == '"' && str[str.size() - 1] == '"') {
            return json(str.substr(1, str.size() - 2));
        }

        if(str[0] == '{') {
            json serialized = json::parse(str);
            json newJson;
            for(auto& [key, value] : serialized.items()) {
                newJson[key] = deserialize_inner(value);
            }

            return newJson;
        }

        if(str[0] == '[') {
            json serialized = json::parse(str);
            json newJson;
            for(const auto& element : serialized) {
                newJson.push_back(deserialize_inner(element));
            }

            return newJson;
        }

        if(str[0] == 'I') {
            return stoi(str.substr(1));
        }

        if(str[0] == 'L') {
            return stoll(str.substr(1));
        }

        if(str[0] == 'F' || str[0] == 'D') {
            return stod(str.substr(1));
        }

        throw runtime_error("Invalid string in serialized JSON!");
    }

    json deserialize(const json& j) {
        assert(j.is_object());
        json newJson;
        for(auto& [key, value] : j.items()) {
            newJson[key] = deserialize_inner(value);
        }

        return newJson;
    }
}
