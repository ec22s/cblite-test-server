#pragma once

#include"Defines.h"

#include <string>
#include <nlohmann/json.hpp>
#include <sstream>
#include INCLUDE_FLEECE(Fleece.h)

namespace value_serializer {
    template<typename T>
    typename std::enable_if<std::is_floating_point<T>::value, std::string>::type
    serialize(T val) {
        std::ostringstream out;
        if(sizeof(T) == 8) {
            out << "D";
            out.precision(std::numeric_limits<double>::digits10);
        } else {
            out << "F";
            out.precision(std::numeric_limits<float>::digits10);
        }

        out << val;
        return out.str();;
    }

    template<typename T>
    typename std::enable_if<std::is_integral<T>::value, std::string>::type
    serialize(T val) {
        std::string retVal;
        if(sizeof(T) == 8) {
            retVal = "L";
        } else {
            retVal = "I";
        }

        retVal += std::to_string(val);
        return retVal;
    }

    std::string serialize(std::nullptr_t n);
    std::string serialize(bool b);
    std::string serialize(const std::string &s);


    std::string serialize(const nlohmann::json& j);
    std::string serialize(const FLValue val);

    nlohmann::json deserialize(const nlohmann::json& j);
}
