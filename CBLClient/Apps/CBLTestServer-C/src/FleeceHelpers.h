#pragma once

#include <fleece/Fleece.h>
#include <string>
#include <nlohmann/json.hpp>

void writeFleece(FLMutableDict dict, const std::string& key, const nlohmann::json& element);
void writeFleece(FLMutableArray array, const nlohmann::json& element);

static constexpr size_t kFormattedISO8601DateMaxSize = 40;
FLSlice FormatISO8601Date(char buf[], int64_t time, bool asUTC);