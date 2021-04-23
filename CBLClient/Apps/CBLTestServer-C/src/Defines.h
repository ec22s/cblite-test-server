#pragma once

#include <string>
#include <fleece/Fleece.h>

inline FLString flstr(const std::string& str) {
    if(str.empty()) {
        return kFLSliceNull;
    }

    return {str.data(), str.size()};
}

inline std::string to_string(FLString str) {
    return std::string(static_cast<const char *>(str.buf), str.size);
}

inline std::string to_string(FLSliceResult sr) {
    std::string retVal(static_cast<const char*>(sr.buf), sr.size);
    FLSliceResult_Release(sr);
    return retVal;
}

#define TRY(logic, err) if(!(logic)) { \
    std::string errMsg = to_string(CBLError_Message(&err)); \
    throw std::domain_error(errMsg); \
}

#ifdef _MSC_VER
#include <direct.h>
constexpr char DIRECTORY_SEPARATOR = '\\';
typedef struct _stat64 cbl_stat_t;
#define cbl_stat _stat64
#define cbl_getcwd _getcwd

inline errno_t cbl_fopen(FILE** fd, const char* path, const char* mode) {
    return fopen_s(fd, path, mode);
}

#else
#include <unistd.h>
constexpr char DIRECTORY_SEPARATOR = '/';
typedef struct stat cbl_stat_t;
#define cbl_stat stat
#define cbl_getcwd getcwd

inline errno_t cbl_fopen(FILE** fd, const char* path, const char* mode) {
    *fd = fopen(path, mode);
    if(!*fd) {
        return errno;
    }

    return 0;
}
#endif

extern const char* const kCBLErrorDomainNames[7];
