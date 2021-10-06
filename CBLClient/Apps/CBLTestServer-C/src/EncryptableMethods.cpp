#include "EncryptableMethods.h"
#include "Defines.h"
#include "MemoryMap.h"
#include "Router.h"

#include INCLUDE_CBL(CouchbaseLite.h)
#include <sstream>
using namespace nlohmann;
using namespace std;

static int NextKid {1};

CryptoContext* CryptoContext::create(const string& algorithm, const string& key) {
    if(algorithm == "xor") {
        return new XorCryptoContext(key);
    }

    throw domain_error(string("Invalid algorith: " + algorithm));
}

static void CryptoContext_EntryDelete(void* ptr) {
    delete (CryptoContext *)ptr;
}

CryptoContext::CryptoContext(const string& algorithm, const string& key) 
    :_algorithm(algorithm)
    ,_key(key)
    ,_kid(string("key") + to_string(NextKid++))
{
}

XorCryptoContext::XorCryptoContext(const string& key)
    :CryptoContext("xor", key)
    ,_key(key)
{

}

string XorCryptoContext::encrypt(const string &input) {
    stringstream ss;
    int index = 0;
    for(const auto& c : input) {
        ss << (c ^ _key[index % _key.size()]);
        index++;
    }

    return ss.str();
}

string XorCryptoContext::decrypt(const string& input, const string& algorithm, const string& kid) {
    if(algorithm != "xor" || kid != this->kid()) {
        throw domain_error("mismatched algorithm or kid");
    }

    return encrypt(input); // XOR decrypt and encrypt are the same
}

#ifdef COUCHBASE_ENTERPRISE

static void CBLEncryptable_EntryDelete(void* ptr) {
    CBLEncryptable_Release((CBLEncryptable *)ptr);
}

#endif

namespace encryptable_methods {
    void encryptable_createValue(json& body, mg_connection* conn) {
#ifdef COUCHBASE_ENTERPRISE
        auto type = body["type"].get<string>();
        CBLEncryptable* value;
        if(type == "String") {
            auto toEncrypt = body["encryptableValue"].get<string>();
            value = CBLEncryptable_CreateWithString(flstr(toEncrypt));
        } else if(type == "Bool") {
            auto toEncrypt = body["encryptableValue"].get<bool>();
            value = CBLEncryptable_CreateWithBool(toEncrypt);
        } else if(type == "Double") {
            auto toEncrypt = body["encryptableValue"].get<double>();
            value = CBLEncryptable_CreateWithDouble(toEncrypt);
        } else if(type == "Float") {
            auto toEncrypt = body["encryptableValue"].get<float>();
            value = CBLEncryptable_CreateWithFloat(toEncrypt);
        } else if(type == "Int") {
            auto toEncrypt = body["encryptableValue"].get<int64_t>();
            value = CBLEncryptable_CreateWithInt(toEncrypt);
        } else if(type == "Null") {
            value = CBLEncryptable_CreateWithNull();
        } else if(type == "UInt") {
            auto toEncrypt = body["encryptableValue"].get<uint64_t>();
            value = CBLEncryptable_CreateWithUInt(toEncrypt);
        } else if(type == "Array") {
            auto toEncrypt = (FLArray)memory_map::get(body["encryptableValue"].get<string>());
            value = CBLEncryptable_CreateWithArray(toEncrypt);
        } else if(type == "Dict") {
            auto toEncrypt = (FLDict)memory_map::get(body["encryptableValue"].get<string>());
            value = CBLEncryptable_CreateWithDict(toEncrypt);
        } else {
            throw domain_error(string("Invalid type passed: " + type));
        }

        write_serialized_body(conn, memory_map::store(value, CBLEncryptable_EntryDelete));
#else
        mg_send_http_error(conn, 501, "Not supported in CE edition");
#endif
    }

    void encryptable_createEncryptor(json& body, mg_connection* conn) {
#ifdef COUCHBASE_ENTERPRISE
        auto algorithm = body["algo"].get<string>();
        auto key = body["key"].get<string>();
        auto* cryptor = CryptoContext::create(algorithm, key);
        write_serialized_body(conn, memory_map::store(cryptor, CryptoContext_EntryDelete));
#else
        mg_send_http_error(conn, 501, "Not supported in CE edition");
#endif
    }

    void encryptable_isEncryptableValue(json& body, mg_connection* conn) {
#ifdef COUCHBASE_ENTERPRISE
        with<CBLDocument *>(body, "document", [&body, conn](CBLDocument *doc) {
            auto key = body["key"].get<string>();
            auto properties = CBLDocument_Properties(doc);
            auto val = FLDict_Get(properties, {key.data(), key.size()});
            write_serialized_body(conn, FLValue_IsEncryptableValue(val));
        });
#else
        mg_send_http_error(conn, 501, "Not supported in CE edition");
#endif
    }
}
