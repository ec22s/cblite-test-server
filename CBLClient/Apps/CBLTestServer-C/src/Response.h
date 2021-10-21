#pragma once

#include <string>
#include <vector>
#include <utility>

struct mg_connection;

class Response final {
public:
    Response();

    int status = 0;
    std::string message;

    void add_header(const std::string& name, const std::string &value);
    void send(mg_connection* conn);

private:
    std::vector<std::pair<std::string, std::string>> _headers;
};