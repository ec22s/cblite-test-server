#pragma once

#include <string>
#include <nlohmann/json.hpp>
#include <fleece/Fleece.h>

namespace value_serializer {
    template<typename T>
    typename std::enable_if<std::is_floating_point<T>::value, std::string>::type
    serialize(T val);

    template<typename T>
    typename std::enable_if<std::is_integral<T>::value, std::string>::type
    serialize(T val);

    std::string serialize(nullptr_t n);
    std::string serialize(bool b);
    std::string serialize(const std::string &s);


    std::string serialize(const nlohmann::json& j);
    std::string serialize(const FLValue val);

    nlohmann::json deserialize(const nlohmann::json& j);
}