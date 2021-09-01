#pragma once

#include <nlohmann/json.hpp>
#include <civetweb.h>
#include <string>

class CryptoContext {
public:
    static CryptoContext* create(const std::string& algorithm, const std::string& key);

    virtual ~CryptoContext() = default;

    const std::string& algorithm() { return _algorithm; }
    const std::string& kid() { return _kid; }

    virtual std::string encrypt(const std::string &input) = 0;
    virtual std::string decrypt(const std::string &input, const std::string& algorithm, const std::string& kid) = 0;

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

    std::string encrypt(const std::string &input) override;
    std::string decrypt(const std::string& input, const std::string& algorithm, const std::string& kid) override;

private:
    std::string _key;
};

namespace encryptable_methods {
    void encryptable_createValue(nlohmann::json& body, mg_connection* conn);
    void encryptable_createEncryptor(nlohmann::json& body, mg_connection* conn);
    void encryptable_isEncryptableValue(nlohmann::json& body, mg_connection* conn);
}