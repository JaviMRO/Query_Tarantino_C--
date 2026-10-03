# Libraries of SPEC §14. Each one is taken from the system when it is installed; otherwise the ones that build with
# CMake are downloaded and built inside the build directory, so the project builds on Linux, macOS and Windows with
# nothing more than a compiler, CMake and ICU (which Windows 10 and later already ship).

include(FetchContent)
if(POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif()
if(POLICY CMP0168)
    # Downloads populate directly, without a sub-build per dependency
    cmake_policy(SET CMP0168 NEW)
endif()

if(MINGW AND NOT DEFINED CMAKE_TLS_CAINFO AND NOT DEFINED ENV{CMAKE_TLS_CAINFO})
    # Some MinGW distributions build CMake with a TLS library that does not read the certificates of Windows, so every
    # download fails its certificate check; Git for Windows, which the downloads need anyway, has the usual bundle.
    # Every download is also pinned by its hash.
    find_package(Git QUIET)
    if(GIT_FOUND)
        execute_process(COMMAND "${GIT_EXECUTABLE}" --exec-path OUTPUT_VARIABLE TARANTINO_GIT_EXEC_PATH
                        OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
        get_filename_component(TARANTINO_CA_BUNDLE "${TARANTINO_GIT_EXEC_PATH}/../../etc/ssl/certs/ca-bundle.crt" ABSOLUTE)
        if(EXISTS "${TARANTINO_CA_BUNDLE}")
            set(CMAKE_TLS_CAINFO "${TARANTINO_CA_BUNDLE}")
            message(STATUS "Downloads checked against the certificates of Git: ${TARANTINO_CA_BUNDLE}")
        endif()
    endif()
endif()

find_package(Threads REQUIRED)

# ---------------------------------------------------------------------------------------------------------------------
# ICU (SPEC §7): lowercasing with the root locale and the letters of general category L, through its C API
# ---------------------------------------------------------------------------------------------------------------------
if(APPLE AND NOT ICU_ROOT AND NOT DEFINED ENV{ICU_ROOT})
    # Homebrew installs icu4c keg-only, out of the default search paths
    find_program(TARANTINO_BREW brew)
    if(TARANTINO_BREW)
        execute_process(COMMAND ${TARANTINO_BREW} --prefix icu4c
                        OUTPUT_VARIABLE TARANTINO_ICU_PREFIX OUTPUT_STRIP_TRAILING_WHITESPACE
                        RESULT_VARIABLE TARANTINO_BREW_RESULT ERROR_QUIET)
        if(TARANTINO_BREW_RESULT EQUAL 0 AND EXISTS "${TARANTINO_ICU_PREFIX}")
            set(ICU_ROOT "${TARANTINO_ICU_PREFIX}")
        endif()
    endif()
endif()

add_library(tarantino_icu INTERFACE)
find_package(ICU 60 QUIET COMPONENTS uc OPTIONAL_COMPONENTS data)
if(ICU_FOUND)
    target_link_libraries(tarantino_icu INTERFACE ICU::uc)
    if(TARGET ICU::data)
        target_link_libraries(tarantino_icu INTERFACE ICU::data)
    endif()
    set(TARANTINO_ICU "ICU ${ICU_VERSION}")
elseif(WIN32)
    # Windows 10 (version 1903) and later ship ICU as icu.dll, with an unversioned C API: Visual Studio links it
    # with icu.lib from the Windows SDK and MinGW-w64 with its libicu.a
    target_link_libraries(tarantino_icu INTERFACE icu)
    target_compile_definitions(tarantino_icu INTERFACE TARANTINO_WINDOWS_ICU=1)
    set(TARANTINO_ICU "the ICU of Windows (icu.dll)")
else()
    message(FATAL_ERROR "ICU was not found. Install it with your package manager: libicu-dev (Debian, Ubuntu), "
                        "libicu-devel (Fedora), icu4c (Homebrew), or point ICU_ROOT to it.")
endif()
message(STATUS "Unicode: ${TARANTINO_ICU}")

# ---------------------------------------------------------------------------------------------------------------------
# SQLite (SPEC §8.2), through its C API
# ---------------------------------------------------------------------------------------------------------------------
if(TARANTINO_SQLITE STREQUAL "SYSTEM")
    find_package(SQLite3 REQUIRED)
    set(TARANTINO_SQLITE_TARGET SQLite::SQLite3)
    message(STATUS "SQLite: system ${SQLite3_VERSION}")
else()
    FetchContent_Declare(sqlite
        URL https://www.sqlite.org/2026/sqlite-amalgamation-3530400.zip
        URL_HASH SHA3_256=628a44cfe82c66aed1ccbbe85a562d2e33ebe64b3288981ed76285612227934e)
    FetchContent_MakeAvailable(sqlite)
    add_library(tarantino_sqlite3 STATIC ${sqlite_SOURCE_DIR}/sqlite3.c)
    target_include_directories(tarantino_sqlite3 SYSTEM PUBLIC ${sqlite_SOURCE_DIR})
    # SQLite's defaults otherwise (SPEC §8.2): rollback journal, synchronous=FULL, autocommit
    target_compile_definitions(tarantino_sqlite3 PRIVATE SQLITE_OMIT_LOAD_EXTENSION SQLITE_THREADSAFE=1)
    target_link_libraries(tarantino_sqlite3 PUBLIC Threads::Threads)
    if(NOT MSVC)
        target_compile_options(tarantino_sqlite3 PRIVATE -w)
    endif()
    set(TARANTINO_SQLITE_TARGET tarantino_sqlite3)
    message(STATUS "SQLite: bundled 3.53.4")
endif()

# ---------------------------------------------------------------------------------------------------------------------
# libcurl (SPEC §4)
# ---------------------------------------------------------------------------------------------------------------------
find_package(CURL QUIET)
if(CURL_FOUND)
    set(TARANTINO_CURL_TARGET CURL::libcurl)
    message(STATUS "libcurl: system ${CURL_VERSION_STRING}")
elseif(WIN32)
    # A static libcurl over Schannel, the TLS of Windows, which needs no other library
    set(BUILD_CURL_EXE OFF CACHE BOOL "" FORCE)
    set(BUILD_LIBCURL_DOCS OFF CACHE BOOL "" FORCE)
    set(BUILD_MISC_DOCS OFF CACHE BOOL "" FORCE)
    set(ENABLE_CURL_MANUAL OFF CACHE BOOL "" FORCE)
    set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(CURL_USE_SCHANNEL ON CACHE BOOL "" FORCE)
    set(CURL_USE_OPENSSL OFF CACHE BOOL "" FORCE)
    set(CURL_USE_LIBPSL OFF CACHE BOOL "" FORCE)
    set(CURL_USE_LIBSSH2 OFF CACHE BOOL "" FORCE)
    set(CURL_ZLIB OFF CACHE STRING "" FORCE)
    set(CURL_BROTLI OFF CACHE STRING "" FORCE)
    set(CURL_ZSTD OFF CACHE STRING "" FORCE)
    set(USE_NGHTTP2 OFF CACHE BOOL "" FORCE)
    set(USE_LIBIDN2 OFF CACHE BOOL "" FORCE)
    set(USE_WIN32_IDN OFF CACHE BOOL "" FORCE)
    set(CURL_DISABLE_INSTALL ON CACHE BOOL "" FORCE)
    set(HTTP_ONLY ON CACHE BOOL "" FORCE)
    set(BUILD_STATIC_LIBS ON CACHE BOOL "" FORCE)
    set(CURL_STATIC_CRT OFF CACHE BOOL "" FORCE)
    set(_tarantino_shared ${BUILD_SHARED_LIBS})
    set(_tarantino_testing ${BUILD_TESTING})
    set(BUILD_SHARED_LIBS OFF)
    set(BUILD_TESTING OFF)
    FetchContent_Declare(curl
        URL https://curl.se/download/curl-8.22.0.tar.xz
        URL_HASH SHA256=f7ef3ae8a22e521f289803fe93543eb64c329b58aa73a9e224dfd915a2a5f4f7)
    FetchContent_MakeAvailable(curl)
    set(BUILD_SHARED_LIBS ${_tarantino_shared})
    set(BUILD_TESTING ${_tarantino_testing})
    set(TARANTINO_CURL_TARGET CURL::libcurl)
    message(STATUS "libcurl: bundled 8.22.0 (Schannel)")
else()
    message(FATAL_ERROR "libcurl was not found. Install it with your package manager: libcurl4-openssl-dev (Debian, "
                        "Ubuntu) or libcurl-devel (Fedora); macOS already has it.")
endif()

# ---------------------------------------------------------------------------------------------------------------------
# nlohmann/json (SPEC §8.1)
# ---------------------------------------------------------------------------------------------------------------------
find_package(nlohmann_json 3.10 CONFIG QUIET)
if(NOT nlohmann_json_FOUND)
    FetchContent_Declare(nlohmann_json
        URL https://github.com/nlohmann/json/releases/download/v3.12.0/json.tar.xz
        URL_HASH SHA256=42f6e95cad6ec532fd372391373363b62a14af6d771056dbfc86160e6dfff7aa)
    FetchContent_MakeAvailable(nlohmann_json)
    message(STATUS "nlohmann/json: bundled 3.12.0")
else()
    message(STATUS "nlohmann/json: system ${nlohmann_json_VERSION}")
endif()

# ---------------------------------------------------------------------------------------------------------------------
# mongocxx (SPEC §8): optional, so the project builds where the MongoDB C++ driver is not installed
# ---------------------------------------------------------------------------------------------------------------------
set(TARANTINO_WITH_MONGO OFF)
if(TARANTINO_MONGO STREQUAL "FETCH")
    # The C++ driver downloads the MongoDB C driver it needs itself
    set(_tarantino_shared ${BUILD_SHARED_LIBS})
    set(BUILD_SHARED_LIBS OFF)
    set(ENABLE_TESTS OFF CACHE BOOL "" FORCE)
    set(ENABLE_UNINSTALL OFF CACHE BOOL "" FORCE)
    set(ENABLE_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(ENABLE_STATIC ON CACHE STRING "" FORCE)
    set(ENABLE_SHARED OFF CACHE BOOL "" FORCE)
    set(ENABLE_SASL OFF CACHE STRING "" FORCE)
    set(ENABLE_SNAPPY OFF CACHE STRING "" FORCE)
    set(ENABLE_ZSTD OFF CACHE STRING "" FORCE)
    set(ENABLE_ZLIB BUNDLED CACHE STRING "" FORCE)
    set(ENABLE_SRV OFF CACHE STRING "" FORCE)
    set(ENABLE_CLIENT_SIDE_ENCRYPTION OFF CACHE STRING "" FORCE)
    set(ENABLE_MONGODB_AWS_AUTH OFF CACHE STRING "" FORCE)
    FetchContent_Declare(mongo-cxx-driver
        URL https://github.com/mongodb/mongo-cxx-driver/releases/download/r4.6.0/mongo-cxx-driver-r4.6.0.tar.gz
        URL_HASH SHA256=eac122db0789fc82b0ba93f92a1503d74c502bfe4728345eaa8650e50a79da11)
    FetchContent_MakeAvailable(mongo-cxx-driver)
    set(BUILD_SHARED_LIBS ${_tarantino_shared})
    add_library(tarantino_mongocxx INTERFACE)
    target_link_libraries(tarantino_mongocxx INTERFACE mongocxx_static)
    set(TARANTINO_WITH_MONGO ON)
    message(STATUS "mongocxx: bundled r4.6.0")
elseif(NOT TARANTINO_MONGO STREQUAL "OFF")
    find_package(mongocxx CONFIG QUIET)
    if(mongocxx_FOUND)
        add_library(tarantino_mongocxx INTERFACE)
        if(TARGET mongo::mongocxx_static AND NOT TARGET mongo::mongocxx_shared)
            target_link_libraries(tarantino_mongocxx INTERFACE mongo::mongocxx_static)
        else()
            target_link_libraries(tarantino_mongocxx INTERFACE mongo::mongocxx_shared)
        endif()
        set(TARANTINO_WITH_MONGO ON)
        message(STATUS "mongocxx: system ${mongocxx_VERSION}")
    elseif(TARANTINO_MONGO STREQUAL "ON")
        message(FATAL_ERROR "mongocxx was not found: install the MongoDB C++ driver or configure with "
                            "-DTARANTINO_MONGO=FETCH to build it from source")
    else()
        message(STATUS "mongocxx: not found, building without MongoDB support (-DTARANTINO_MONGO=FETCH builds it)")
    endif()
endif()
if(TARANTINO_WITH_MONGO)
    target_compile_definitions(tarantino_mongocxx INTERFACE TARANTINO_WITH_MONGO=1)
endif()

# ---------------------------------------------------------------------------------------------------------------------
# GoogleTest (SPEC §17)
# ---------------------------------------------------------------------------------------------------------------------
if(TARANTINO_BUILD_TESTS)
    find_package(GTest 1.10 CONFIG QUIET)
    if(NOT GTest_FOUND)
        set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
        set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
        set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
        FetchContent_Declare(googletest
            URL https://github.com/google/googletest/releases/download/v1.18.0/googletest-1.18.0.tar.gz
            URL_HASH SHA256=6e3191c1455468b3fc35a417fb565c1c5071aee1b7e7f85e30cf48a98d37d8b5)
        FetchContent_MakeAvailable(googletest)
        message(STATUS "GoogleTest: bundled 1.18.0")
    else()
        message(STATUS "GoogleTest: system ${GTest_VERSION}")
    endif()
endif()
