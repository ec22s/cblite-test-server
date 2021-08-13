#include "DataTypeMethods.h"
#include "MemoryMap.h"
#include "Router.h"
#include "Defer.hh"
#include "FleeceHelpers.h"
#include "Defines.h"
#include "FilePathResolver.h"
#include "date.h"

#include INCLUDE_CBL(CouchbaseLite.h)
using namespace std;
using namespace nlohmann;
using namespace chrono;
using namespace date;

void DateTime_EntryDelete(void* ptr);

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
        auto now = new milliseconds(duration_cast<milliseconds>(system_clock::now().time_since_epoch()));
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
        auto first = body["first"].get<int>();
        auto second = body["second"].get<int>();
        write_serialized_body(conn, first == second);
    }

    void datatype_compareDate(json& body, mg_connection* conn) {
        auto date1 = (milliseconds *)memory_map::get(body["date1"].get<string>());
        auto date2 = (milliseconds *)memory_map::get(body["date2"].get<string>());
        write_serialized_body(conn, *date1 == *date2);
    }
}