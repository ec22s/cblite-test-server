#pragma once

#include <string>
#include <nlohmann/json.hpp>
#include <fleece/Fleece.h>

namespace value_serializer {
    std::string serialize(const nlohmann::json& j);
    std::string serialize(const FLValue val);

    nlohmann::json deserialize(const nlohmann::json& j);
}