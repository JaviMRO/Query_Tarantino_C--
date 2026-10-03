cmake_minimum_required(VERSION 3.21)

project(QueryTarantino
        VERSION 1.0.0
        DESCRIPTION "Query Tarantino Stage 1: the data layer of a search engine over Project Gutenberg books"
        LANGUAGES C CXX)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Release (-O2 -DNDEBUG, SPEC §11) or Debug" FORCE)
endif()

option(TARANTINO_BUILD_TESTS "Build the tests and the benchmarks" ON)
set(TARANTINO_MONGO AUTO CACHE STRING
    "MongoDB support (index and metadata 'mongo'): AUTO uses mongocxx when it is installed, FETCH builds it from source, ON requires it, OFF leaves it out")
set_property(CACHE TARANTINO_MONGO PROPERTY STRINGS AUTO FETCH ON OFF)
set(TARANTINO_SQLITE BUNDLED CACHE STRING
    "SQLite: BUNDLED builds 3.53.4, the version Java's sqlite-jdbc bundles (SPEC §11); SYSTEM uses the installed one")
set_property(CACHE TARANTINO_SQLITE PROPERTY STRINGS BUNDLED SYSTEM)

# Every executable in one place, so the services run as <build>/bin/<service> from the project root (SPEC §15); the
# empty generator expression keeps multi-configuration generators (Visual Studio, Xcode) from adding a Release folder
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin$<0:>)
list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

include(CompilerOptions)
include(Dependencies)

if(TARANTINO_BUILD_TESTS)
    enable_testing()
    include(GoogleTest)
endif()

add_subdirectory(platform)
add_subdirectory(services/crawler)
add_subdirectory(services/indexer)
add_subdirectory(services/query)
add_subdirectory(services/control)

if(TARANTINO_BUILD_TESTS)
    # The benchmarks of every service, one after the other, from the project root (BENCHMARKS.md): what
    # mvn verify -Pbenchmark is for Java
    add_custom_target(benchmarks
        COMMAND crawler_benchmarks
        COMMAND indexer_benchmarks
        COMMAND query_benchmarks
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
        USES_TERMINAL
        COMMENT "Running the benchmarks of the crawler, the indexer and the query service")
endif()

message(STATUS "Query Tarantino: MongoDB support ${TARANTINO_WITH_MONGO}, tests and benchmarks ${TARANTINO_BUILD_TESTS}")
