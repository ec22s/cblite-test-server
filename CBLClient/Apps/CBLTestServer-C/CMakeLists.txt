cmake_minimum_required(VERSION 3.11)
cmake_policy(VERSION 3.11)
cmake_policy(SET CMP0077 NEW)

# Mac/apple setup -- must appear before the first "project()" line"
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.12")
if(NOT DEFINED CMAKE_OSX_SYSROOT)
    # Tells Mac builds to use the current SDK's headers & libs, not what's in the OS.
    set(CMAKE_OSX_SYSROOT macosx)
endif()

project(testsuite)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)

add_subdirectory(shared)

get_directory_property(civetweb_SOURCE_DIR DIRECTORY shared DEFINITION civetweb_SOURCE_DIR)

find_package(CouchbaseLite 3.0.0 REQUIRED)

add_executable(
    testserver
    src/TestServer.cpp
    src/Router.cpp
    src/Response.cpp
    src/ValueSerializer.cpp
    src/MemoryMap.cpp
    src/main.cpp
    src/ArrayMethods.cpp
)

target_link_libraries(
    testserver PRIVATE
    civetweb-c-library
    nlohmann_json::nlohmann_json
    CouchbaseLiteC
)

target_include_directories(
    testserver PRIVATE
    ${civetweb_SOURCE_DIR}/include
)

target_compile_definitions(
    testserver PRIVATE
    -DJSON_USE_IMPLICIT_CONVERSIONS=0
)

if(MSVC)
    target_compile_options(
        testserver PRIVATE
        -D_WIN32_WINNT=0x0A00
        -DWIN32_LEAN_AND_MEAN
    )
endif()