#pragma once

#include <nlohmann/json.hpp>
#include <civetweb.h>
#include <string>
#include "Defines.h"

#include INCLUDE_FLEECE(Fleece.h)

class CryptoContext {
public:
    static CryptoContext* create(const std::string& algorithm, const std::string& key);

    virtual ~CryptoContext() = default;

    const std::string& algorithm() { return _algorithm; }
    const std::string& kid() { return _kid; }

    virtual FLSliceResult encrypt(FLSlice input) = 0;
    virtual FLSliceResult decrypt(FLSlice input, const std::string& algorithm, const std::string& kid) = 0;

protected:
    CryptoContext(const std::string& algorithm, const std::string& key);

    const std::string& key() { return _key; }
private:
    std::string _algorithm;
    std::string _kid;
    std::string _key;
};

class XorCryptoContext final : public CryptoContext {
public:
    XorCryptoContext(const std::string& key);

    virtual ~XorCryptoContext() = default;

    FLSliceResult encrypt(FLSlice input) override;
    FLSliceResult decrypt(FLSlice input, const std::string& algorithm, const std::string& kid) override;

private:
    std::string _key;
};

namespace encryptable_methods {
    void encryptable_createValue(nlohmann::json& body, mg_connection* conn);
    void encryptable_createEncryptor(nlohmann::json& body, mg_connection* conn);
    void encryptable_isEncryptableValue(nlohmann::json& body, mg_connection* conn);
}