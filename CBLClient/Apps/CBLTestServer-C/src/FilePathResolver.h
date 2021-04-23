#pragma once

#include <string>

namespace file_resolution {
    std::string resolve_path(const std::string& relativePath, bool unzip);
}