#include "FleeceHelpers.h"

#include "date.h"

#include <iomanip>
#include <mutex>

#ifndef WINAPI_FAMILY_PARTITION
#ifdef _MSC_VER
#include <winapifamily.h>
#else
#define WINAPI_FAMILY_PARTITION(x) false
#endif
#endif

using namespace std;
using namespace nlohmann;
using namespace date;
using namespace std::chrono;

using value_t = nlohmann::detail::value_t;

static void addElement(FLSlot slot, const json& element) {
    switch(element.type()) {
        case value_t::number_float:
            FLSlot_SetDouble(slot, element.get<double>());
            break;
        case value_t::number_integer:
            FLSlot_SetInt(slot, element.get<int64_t>());
            break;
        case value_t::number_unsigned:
            FLSlot_SetUInt(slot, element.get<uint64_t>());
            break;
        case value_t::string:
        {
            auto s = element.get<string>();
            FLString val = { s.data(), s.size() };
            FLSlot_SetString(slot, val);
            break;
        }
        case value_t::boolean:
            FLSlot_SetBool(slot, element.get<bool>());
            break;
        case value_t::null:
            FLSlot_SetNull(slot);
            break;
        case value_t::array:
        {
            FLMutableArray nextArr = FLMutableArray_New();
            for(const auto& subelement : element) {
                writeFleece(nextArr, subelement);
            }

            FLSlot_SetArray(slot, nextArr);
            FLMutableArray_Release(nextArr);
            break;
        }
        case value_t::object:
        {
            FLMutableDict nextDict = FLMutableDict_New();
            for(const auto& [key, value] : element.items()) {
                writeFleece(nextDict, key, value);
            }

            FLSlot_SetDict(slot, nextDict);
            FLMutableDict_Release(nextDict);
            break;
        }
        case value_t::binary:
        case value_t::discarded:
        default:
            throw domain_error("Invalid JSON entry in writeFleece!");
    }
}

void writeFleece(FLMutableArray array, const json& element) {
    FLSlot slot = FLMutableArray_Append(array);
    addElement(slot, element);
}

void writeFleece(FLMutableDict dict, const string& key, const json& element) {
    FLString keySlice = { key.data(), key.size() };
    FLSlot slot = FLMutableDict_Set(dict, keySlice);
    addElement(slot, element);
}

static struct tm FromTimestamp(seconds timestamp) {
    local_seconds tp { timestamp };
    auto dp = floor<days>(tp);
    year_month_day ymd { dp };
    auto hhmmss = make_time(tp - dp);

    struct tm local_time {};
    local_time.tm_sec = (int)hhmmss.seconds().count();
    local_time.tm_min = (int)hhmmss.minutes().count();
    local_time.tm_hour = (int)hhmmss.hours().count();
    local_time.tm_mday = (int)static_cast<unsigned>(ymd.day());
    local_time.tm_mon = (int)static_cast<unsigned>(ymd.month()) - 1;
    local_time.tm_year = static_cast<int>(ymd.year()) - 1900;
    local_time.tm_isdst = -1;

    return local_time;
}

static seconds GetLocalTZOffset(struct tm* localtime, bool input_utc) {
    // This method is annoyingly delicate, and warrants lots of explanation

    // First, call tzset so that the needed information is populated
    // by the C runtime
#if !defined(_MSC_VER) || WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
    // Let's hope this works on UWP since Microsoft has removed the
    // tzset and _tzset functions from UWP
    static std::once_flag once;
    std::call_once(once, []
    {
#ifdef _MSC_VER
        _tzset();
#else
        tzset();
#endif
    });
#endif

    // Find the system time zone's offset from UTC
    // Windows -> _get_timezone
    // Others -> global timezone variable
    //      https://linux.die.net/man/3/tzset (System V-like / XSI)
    //      http://www.unix.org/version3/apis/t_9.html (Unix v3)
    //
    // NOTE: These values are the opposite of what you would expect, being defined
    // as seconds WEST of GMT (so UTC-8 would be 28,800, not -28,800)
#ifdef WIN32
    long s;
    if(_get_timezone(&s) != 0) {
        throw runtime_error("Unable to query local system time zone");
    }

    auto offset = seconds(-s);
#elif defined(__DARWIN_UNIX03) || defined(__ANDROID__) || defined(_XOPEN_SOURCE) || defined(_SVID_SOURCE)
    auto offset = seconds(-timezone);
#else
    #error Unimplemented GetLocalTZOffset
#endif

    // Apply the timezone offset first to get the proper time
    // in the current timezone (no-op if local time was passed)
    if(input_utc) {
        localtime->tm_sec -= static_cast<int>(offset.count());
    }

    // In order to consider DST, mktime needs to be called.
    // However this has the caveat that it will never be
    // clear if the "before" or "after" DST is desired in the case
    // of a rollback of clocks in which an hour is repeated.  Moral
    // of the story:  USE TIME ZONES IN YOUR DATE STRINGS!
    if(mktime(localtime) != -1) {
        offset += hours(localtime->tm_isdst);
    }

    return offset;
}

FLSlice FormatISO8601Date(char buf[], int64_t time, bool asUTC) {
    if (time == INT64_MIN) {
        *buf = 0;
        return kFLSliceNull;
    }

    stringstream timestream;
    auto tp = local_time<milliseconds>(milliseconds(time));
    auto totalSec = floor<seconds>(tp);
    auto totalMs = floor<milliseconds>(tp);
    struct tm tmpTime = FromTimestamp(totalSec.time_since_epoch());
    auto offset = GetLocalTZOffset(&tmpTime, true);
    if(asUTC || offset == 0min) {
        if(totalSec == totalMs) {
            timestream << format("%FT%TZ", floor<seconds>(tp));
        } else {
            timestream << format("%FT%TZ", floor<milliseconds>(tp));
        }
    } else {
        tp += offset;
        auto h = duration_cast<hours>(offset);
        auto m = offset - h;
        timestream.fill('0');
        if(totalSec == totalMs) {
            timestream << format("%FT%T", floor<seconds>(tp));
        } else {
            timestream << format("%FT%T", floor<milliseconds>(tp));
        }
           
        timestream << setw(3) << std::internal << showpos << h.count()
            << noshowpos << setw(2) << (int)abs(m.count());
    }
    
    memcpy(buf, timestream.str().c_str(), timestream.str().size());
    buf[timestream.str().length()] = 0;
    return {buf, timestream.str().length()};
}

FLValue FLMutableDict_FindValue(FLMutableDict dict, const string& key, FLValueType type) {
    if(FLDict_Count(dict) == 0) {
        return nullptr;
    }

    const auto* flVal = FLDict_Get(dict, { key.data(), key.size() });
    if(!flVal) {
        return nullptr;
    }

    return (type == kFLUndefined || FLValue_GetType(flVal) == type) ? flVal : nullptr;
}

FLValue FLDict_FindValue(FLDict dict, const string& key, FLValueType type) {
    if(FLDict_Count(dict) == 0) {
        return nullptr;
    }

    const auto* flVal = FLDict_Get(dict, { key.data(), key.size() });
    if(!flVal) {
        return nullptr;
    }

    return (type == kFLUndefined || FLValue_GetType(flVal) == type) ? flVal : nullptr;
}
