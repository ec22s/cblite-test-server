#include "Response.h"
#include <civetweb.h>
using namespace std;

Response::Response() {
    _headers.emplace_back(make_pair("Server", "QE TestServer C"));
}

void Response::add_header(const std::string& name, const std::string &value) {
    _headers.emplace_back(make_pair(name, value));
}

void Response::send(mg_connection* conn) {
    mg_response_header_start(conn, status);
    for(const auto& hdr : _headers) {
        mg_response_header_add(conn, hdr.first.c_str(), hdr.second.c_str(), hdr.second.size());
    }

    mg_response_header_send(conn);
    mg_close_connection(conn);
}
