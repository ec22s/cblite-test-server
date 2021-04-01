#pragma once

#include <string>
#include <functional>
#include <nlohmann/json.hpp>

namespace memory_map {
    using cleanup_func = std::function<void(void *)>;

    void init();
    void clear();
    void* get(const std::string &id);
    std::string store(void* item, cleanup_func cleanup);
    void release(const std::string &id);
}

template<typename T>
void with(nlohmann::json& postBody, const std::string& key, std::function<void(T)> action) {
    const auto handle = postBody[key].get<std::string>();
    auto obj = static_cast<T>(memory_map::get(handle));
    action(obj);
}