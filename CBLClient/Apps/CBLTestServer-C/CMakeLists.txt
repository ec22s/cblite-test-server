cmake_minimum_required(VERSION 3.11)
cmake_policy(VERSION 3.11)
cmake_policy(SET CMP0077 NEW)

# Mac/apple setup -- must appear before the first "project()" line"
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.12")
if(NOT DEFINED CMAKE_OSX_SYSROOT)
    # Tells Mac builds to use the current SDK's headers & libs, not what's in the OS.
    set(CMAKE_OSX_SYSROOT macosx)
endif()

if(DEFINED CBL_MACOS_ARCH)
    set(CMAKE_OSX_ARCHITECTURES ${CBL_MACOS_ARCH})
endif()

project(testsuite)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)
set(CMAKE_INSTALL_PREFIX ${CMAKE_CURRENT_BINARY_DIR}/out)

# if(WINDOWS_STORE)
#     set_property(GLOBAL PROPERTY USE_FOLDERS ON)
#     set(PACKAGE_GUID "3f18ac77-9071-4c4d-beed-9f33b982780c")

#     file(COPY UWP/Package.appxmanifest DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
#     set(CONTENT_FILES ${CMAKE_CURRENT_BINARY_DIR}/Package.appxmanifest)
#     set(
#         ASSET_FILES
#         UWP/Assets/LockScreenLogo.scale-200.png
#         UWP/Assets/SplashScreen.scale-200.png
#         UWP/Assets/Square150x150Logo.scale-200.png
#         UWP/Assets/Square44x44Logo.scale-200.png
#         UWP/Assets/Square44x44Logo.targetsize-24_altform-unplated.png
#         UWP/Assets/StoreLogo.png
#         UWP/Assets/Wide310x150Logo.scale-200.png
#     )

#     set(RESOURCE_FILES ${CONTENT_FILES} ${ASSET_FILES} UWP/UWP_TemporaryKey.pfx)

#     set_property(SOURCE ${CONTENT_FILES} PROPERTY VS_DEPLOYMENT_CONTENT 1)
#     set_property(SOURCE ${ASSET_FILES} PROPERTY VS_DEPLOYMENT_CONTENT 1)
#     set_property(SOURCE ${ASSET_FILES} PROPERTY VS_DEPLOYMENT_LOCATION "Assets")
# endif()

add_subdirectory(shared)

get_directory_property(civetweb_SOURCE_DIR DIRECTORY shared DEFINITION civetweb_SOURCE_DIR)

if(ANDROID)
    set( CMAKE_FIND_ROOT_PATH_MODE_PACKAGE NEVER )
endif()

message(STATUS ${CMAKE_LIBRARY_ARCHITECTURE})
find_package(CouchbaseLite 3.0.0 REQUIRED)

if(ANDROID)
    set( CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY )
endif()

if(APPLE)
    set(CMAKE_INSTALL_RPATH "@loader_path")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(CMAKE_INSTALL_RPATH "\$ORIGIN")
    if(CBL_STATIC_CXX)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static-libstdc++")
    endif()
endif()

set(
    TESTSERVER_COMMON_SRC
    src/TestServer.cpp
    src/Router.cpp
    src/Response.cpp
    src/ValueSerializer.cpp
    src/MemoryMap.cpp
    src/FleeceHelpers.cpp
    src/ArrayMethods.cpp
    src/BasicAuthenticationMethods.cpp
    src/BlobMethods.cpp
    src/DatabaseMethods.cpp
    src/DatabaseConfigurationMethods.cpp
    src/DataTypeMethods.cpp
    src/DictionaryMethods.cpp
    src/DocumentMethods.cpp
    src/EncryptableMethods.cpp
    src/FileLoggingMethods.cpp
    src/ReplicatorMethods.cpp
    src/ReplicatorConfigurationMethods.cpp
    src/SessionAuthenticationMethods.cpp
    src/QueryMethods.cpp
)

if(ANDROID)
    add_library(testserver SHARED ${TESTSERVER_COMMON_SRC})

    # Android prior to API24 is dumb about loading shared libraries
    # and since libtestserver.so depends in CBL-C shared, it cannot
    # load via native activity.  Create this dummy loader to do
    # the actual loading...
    add_library(dummyloader SHARED src/Android/DummyLoader.cpp)

    target_sources(
        testserver PRIVATE
        src/Android/MainActivity.cpp
        src/Android/getifaddrs.cc
        src/Android/bionic_netlink.cc
        src/Android/FilePathResolver+Android.cpp
    )

    add_library(
        native_app_glue STATIC
        ${ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c
    )

    target_include_directories(
        native_app_glue PUBLIC
        ${ANDROID_NDK}/sources/android/native_app_glue
    )

    find_library(log-lib log)

    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -u ANativeActivity_onCreate")
    target_link_libraries(
        testserver PRIVATE
        native_app_glue
        android
        ${log-lib}
    )

    target_link_libraries(
        dummyloader PRIVATE
        native_app_glue
        android
        ${log-lib}
    )
# elseif(WINDOWS_STORE)
#     add_library(testserver SHARED ${TESTSERVER_COMMON_SRC})

#     target_sources(
#         testserver PRIVATE
#         src/UWP/main.cpp
#         src/UWP/FilePathResolver+UWP.cpp
#     )

#     set_target_properties(testserver PROPERTIES COMPILE_FLAGS /ZW)
#     set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /nodefaultlib:kernel32.lib /nodefaultlib:ole32.lib")

#     add_executable(
#         testapp WIN32
#         UWP/App.xaml.cpp
#     )
else()
    add_executable(testserver ${TESTSERVER_COMMON_SRC})

    if(APPLE)
        target_sources(
            testserver PRIVATE
            src/FileLoggingMethods+Apple.mm
        )
    endif()

    if(CMAKE_COMPILER_IS_GNUCC)
        target_compile_options(testserver PRIVATE -Wno-psabi)
        if(CBL_STATIC_CXX)
            target_compile_options(testserver PRIVATE -static-libstdc++)
        endif()
    endif()

    target_sources(
        testserver PRIVATE
        src/main.cpp
        src/FilePathResolver+Desktop.cpp
    )
endif()

target_link_libraries(
    testserver PRIVATE
    civetweb-c-library
    nlohmann_json::nlohmann_json
    cblite
    zip
)

target_include_directories(
    testserver PRIVATE
    ${civetweb_SOURCE_DIR}/include

)

target_compile_definitions(
    testserver PRIVATE
    -DJSON_USE_IMPLICIT_CONVERSIONS=0
    -DHAS_UNCAUGHT_EXCEPTIONS=1
)

if(MSVC)
    target_compile_options(
        testserver PRIVATE
        -D_WIN32_WINNT=0x0A00
        -DWIN32_LEAN_AND_MEAN
    )

    target_link_libraries(
        testserver PRIVATE
        zlibstatic
    )

    target_include_directories(
        testserver PRIVATE
        src/MSVC
    )
endif()

install(
    TARGETS testserver
    RUNTIME DESTINATION bin
)
