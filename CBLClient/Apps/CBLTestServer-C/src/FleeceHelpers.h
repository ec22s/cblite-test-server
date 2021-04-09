#pragma once

#include <fleece/Fleece.h>
#include <string>
#include <nlohmann/json.hpp>

void writeFleece(FLMutableDict dict, const std::string& key, const nlohmann::json& element);
void writeFleece(FLMutableArray array, const nlohmann::json& element);

FLValue FLMutableDict_FindValue(FLMutableDict dict, const std::string& key, FLValueType type);
FLValue FLDict_FindValue(FLDict dict, const std::string& key, FLValueType type);

static constexpr size_t kFormattedISO8601DateMaxSize = 40;
FLSlice FormatISO8601Date(char buf[], int64_t time, bool asUTC);