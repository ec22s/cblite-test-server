#pragma once

#define STRINGIFY(X) STRINGIFY2(X)
#define STRINGIFY2(X) #X
#define APP_EXTENSIONS_DIR "Extensions"

// Thanks Apple, this sorcery is needed to use frameworks include paths
// (i.e. CouchbaseLite/<path>) work as well as normal include paths
// (i.e. cbl/<path> or fleece/<path>)
#ifdef __APPLE__
#include <TargetConditionals.h>
#if !TARGET_OS_OSX
#define INCLUDE_CBL(X) STRINGIFY(CouchbaseLite/X)
#define INCLUDE_FLEECE(X) STRINGIFY(CouchbaseLite/X)
#else
#define INCLUDE_CBL(X) STRINGIFY(cbl/X)
#define INCLUDE_FLEECE(X) STRINGIFY(fleece/X)
#endif
#else
#define INCLUDE_CBL(X) STRINGIFY(cbl/X)
#define INCLUDE_FLEECE(X) STRINGIFY(fleece/X)
#endif

#include <string>
#include <errno.h>
#include INCLUDE_FLEECE(Fleece.h)

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

#define TRY(logic, err) if(!(logic) && err.code != 0) { \
    std::string errMsg = to_string(CBLError_Message(&err)); \
    throw std::domain_error(errMsg); \
}

#ifdef _MSC_VER
#include <direct.h>
#include <io.h>

#define NOMINMAX
#include <Windows.h>
constexpr char DIRECTORY_SEPARATOR = '\\';
typedef struct _stat64 cbl_stat_t;
#define cbl_stat _stat64
#define cbl_getcwd _getcwd

inline errno_t cbl_fopen(FILE** fd, const char* path, const char* mode) {
    HANDLE hFile = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    int opened = _open_osfhandle((intptr_t)hFile, 0);
    if(opened == -1) {
        return errno;
    }

    *fd = _fdopen(opened, mode);
    if(!*fd) {
        _close(opened);
        return errno;
    }

    return 0;
}

inline int cbl_mkdir(const char* path, ...) {
    return _mkdir(path);
}

#else
#include <unistd.h>
constexpr char DIRECTORY_SEPARATOR = '/';
typedef struct stat cbl_stat_t;
#define cbl_stat stat
#define cbl_getcwd getcwd
#define cbl_mkdir mkdir

inline int cbl_fopen(FILE** fd, const char* path, const char* mode) {
    *fd = fopen(path, mode);
    if(!*fd) {
        return errno;
    }

    return 0;
}
#endif

extern const char* const kCBLErrorDomainNames[7];
