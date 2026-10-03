# Query: searches the datamarts (SPEC §10)

add_library(tarantino_query STATIC
    src/QueryConfig.cpp
    src/QueryFactory.cpp
    src/adapters/index/folders/FolderPerTermIndexReader.cpp
    src/adapters/index/folders/TermFiles.cpp
    src/adapters/index/json/JsonIndexFile.cpp
    src/adapters/index/json/MonolithicJsonIndexReader.cpp
    src/adapters/metadata/SqliteMetadataReader.cpp
    src/adapters/stopwords/FileStopwordsLoader.cpp
    src/commands/SearchCommand.cpp
    src/model/QueryTerms.cpp)
target_include_directories(tarantino_query PUBLIC include PRIVATE src)
target_link_libraries(tarantino_query
    PUBLIC tarantino_platform
    PRIVATE ${TARANTINO_SQLITE_TARGET} nlohmann_json::nlohmann_json)
if(TARANTINO_WITH_MONGO)
    target_sources(tarantino_query PRIVATE
        src/adapters/index/mongo/MongodbIndexReader.cpp
        src/adapters/metadata/MongodbMetadataReader.cpp
        src/adapters/mongo/MongoDatabases.cpp)
    target_link_libraries(tarantino_query PUBLIC tarantino_mongocxx tarantino_platform_mongo)
endif()

add_executable(query src/main.cpp)
target_link_libraries(query PRIVATE tarantino_query)

if(NOT TARANTINO_BUILD_TESTS)
    return()
endif()

# The support code of the query benchmarks, on top of the one of the indexer benchmarks (SPEC §11)
add_library(tarantino_query_benchmark_support STATIC
    tests/benchmarking/QueryWorkload.cpp
    tests/benchmarking/ReferenceResults.cpp
    tests/benchmarking/WorkloadQuery.cpp)
target_include_directories(tarantino_query_benchmark_support PUBLIC ${PROJECT_SOURCE_DIR}/services)
target_link_libraries(tarantino_query_benchmark_support PUBLIC tarantino_query tarantino_indexer_benchmark_support)

add_executable(query_benchmarks
    tests/benchmarking/BenchmarkRunner.cpp
    tests/benchmarking/IndexOpenBenchmark.cpp
    tests/benchmarking/MetadataQueryBenchmark.cpp
    tests/benchmarking/QueryTimeBenchmark.cpp
    $<TARGET_OBJECTS:tarantino_allocation_counter>)
target_link_libraries(query_benchmarks PRIVATE
    tarantino_query_benchmark_support ${TARANTINO_SQLITE_TARGET} nlohmann_json::nlohmann_json)

add_executable(query_tests
    tests/QueryFactoryTest.cpp
    tests/adapters/index/folders/FolderPerTermIndexReaderTest.cpp
    tests/adapters/index/folders/FoldersNamesConformanceTest.cpp
    tests/adapters/index/json/MonolithicJsonIndexReaderTest.cpp
    tests/adapters/metadata/SqliteMetadataReaderTest.cpp
    tests/adapters/stopwords/FileStopwordsLoaderTest.cpp
    tests/benchmarking/WorkloadQueryTest.cpp
    tests/commands/SearchCommandTest.cpp
    tests/conformance/QueryConformanceTest.cpp
    tests/model/QueryTermsTest.cpp)
target_include_directories(query_tests PRIVATE ${PROJECT_SOURCE_DIR}/services)
target_link_libraries(query_tests PRIVATE
    tarantino_query tarantino_query_benchmark_support tarantino_crawler_conformance tarantino_testing ${TARANTINO_SQLITE_TARGET}
    GTest::gtest_main)
if(TARANTINO_WITH_MONGO)
    target_sources(query_tests PRIVATE tests/adapters/mongo/MongodbReadersTest.cpp)
    target_link_libraries(query_tests PRIVATE tarantino_indexer_test_mongo)
endif()
gtest_discover_tests(query_tests DISCOVERY_MODE PRE_TEST NO_PRETTY_VALUES
                     PROPERTIES ENVIRONMENT "TARANTINO_WORKLOAD=${PROJECT_SOURCE_DIR}/workload")
