cmake_minimum_required(VERSION 3.11)
cmake_policy(VERSION 3.11)
cmake_policy(SET CMP0077 NEW)

# Mac/apple setup -- must appear before the first "project()" line"
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.12")
if(NOT DEFINED CMAKE_OSX_SYSROOT)
    # Tells Mac builds to use the current SDK's headers & libs, not what's in the OS.
    set(CMAKE_OSX_SYSROOT macosx)
endif()

project(testserver)

include(FetchContent)

FetchContent_Declare(
    civetweb
    GIT_REPOSITORY https://github.com/civetweb/civetweb
    GIT_TAG v1.13
    GIT_SHALLOW TRUE
)

# Downstream does not support CMP0077 NEW
# so need these redundant options with the actual
# value I want
option(CIVETWEB_SERVE_NO_FILES "Configures the server to serve no static files" ON)
option(CIVETWEB_DISABLE_CGI "Disables CGI, so theserver will not execute CGI scripts" ON)
option(CIVETWEB_DISABLE_CACHING "Disables caching, so that no timegm is used." ON)
option(CIVETWEB_ENABLE_SSL "Enables the secure socket layer" OFF)
option(CIVETWEB_ENABLE_ASAN "Enable ASAN in debug mode" OFF)


message(STATUS "Fetching civetweb...")
FetchContent_MakeAvailable(civetweb)

FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/ArthurSonzogni/nlohmann_json_cmake_fetchcontent
    GIT_TAG v3.9.1
    GIT_SHALLOW TRUE
)

message(STATUS "Fetching json...")
FetchContent_MakeAvailable(json)