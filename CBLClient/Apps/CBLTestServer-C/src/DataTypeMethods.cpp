#include "DataTypeMethods.h"
#include "MemoryMap.h"
#include "Router.h"
#include "Defer.hh"
#include "FleeceHelpers.h"
#include "Defines.h"
#include "FilePathResolver.h"
#include <chrono>

#include INCLUDE_CBL(CouchbaseLite.h)
using namespace std;
using namespace nlohmann;
using datetime = chrono::time_point<chrono::system_clock>;

void DateTime_EntryDelete(void* ptr) {
    delete (datetime *)ptr;
}

namespace datatype_methods {
    void datatype_setLong(json& body, mg_connection* conn) {
        auto val = body["value"].get<int64_t>();
        write_serialized_body(conn, val);
    }

    void datatype_setDouble(json& body, mg_connection* conn) {
        auto val = body["value"].get<double>();
        write_serialized_body(conn, val);
    }

    void datatype_setFloat(json& body, mg_connection* conn) {
        auto val = body["value"].get<float>();
        write_serialized_body(conn, val);
    }

    void datatype_setDate(json& body, mg_connection* conn) {
        auto now = new datetime(chrono::system_clock::now());
        write_serialized_body(conn, memory_map::store(now, DateTime_EntryDelete));
    }

    void datatype_compareLong(json& body, mg_connection* conn) {
        auto first = body["first"].get<int64_t>();
        auto second = body["second"].get<int64_t>();
        write_serialized_body(conn, first == second);
    }

    void datatype_compareDouble(json& body, mg_connection* conn) {
        auto first = body["double1"].get<double>();
        auto second = body["double2"].get<double>();
        write_serialized_body(conn, abs(first - second) < numeric_limits<double>::epsilon());
    }

    void datatype_compare(json& body, mg_connection* conn) {
        auto first = body["first"].get<string>();
        auto second = body["second"].get<string>();
        write_serialized_body(conn, first == second);
    }

    void datatype_compareDate(json& body, mg_connection* conn) {
        auto date1 = (datetime *)memory_map::get(body["date1"].get<string>());
        auto date2 = (datetime *)memory_map::get(body["date2"].get<string>());
        write_serialized_body(conn, *date1 == *date2);
    }
}