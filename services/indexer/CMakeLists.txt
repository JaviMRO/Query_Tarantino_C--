# Indexer: reads the datalake and builds the inverted index and the metadata datamart (SPEC §7-§8)

add_library(tarantino_indexer STATIC
    src/IndexerConfig.cpp
    src/IndexerFactory.cpp
    src/adapters/datalake/BookFiles.cpp
    src/adapters/datalake/batch/BatchBasedDatalakeReader.cpp
    src/adapters/datalake/book/BookBasedDatalakeReader.cpp
    src/adapters/datalake/time/TimeBasedDatalakeReader.cpp
    src/adapters/index/PendingPostings.cpp
    src/adapters/index/folders/FolderPerTermIndexAdapter.cpp
    src/adapters/index/folders/TermFiles.cpp
    src/adapters/index/json/JsonIndexFile.cpp
    src/adapters/index/json/MonolithicJsonIndexAdapter.cpp
    src/adapters/metadata/SqliteMetadataAdapter.cpp
    src/adapters/stopwords/FileStopwordsLoader.cpp
    src/commands/IndexBookCommand.cpp
    src/model/book/HeaderParser.cpp
    src/model/terms/Tokenizer.cpp)
target_include_directories(tarantino_indexer PUBLIC include PRIVATE src)
target_link_libraries(tarantino_indexer
    PUBLIC tarantino_platform
    PRIVATE tarantino_icu ${TARANTINO_SQLITE_TARGET} nlohmann_json::nlohmann_json)
if(TARANTINO_WITH_MONGO)
    target_sources(tarantino_indexer PRIVATE
        src/adapters/index/mongo/MongodbIndexAdapter.cpp
        src/adapters/metadata/MongodbMetadataAdapter.cpp
        src/adapters/mongo/MongoDatabases.cpp)
    target_link_libraries(tarantino_indexer PUBLIC tarantino_mongocxx tarantino_platform_mongo)
endif()

add_executable(indexer src/main.cpp)
target_link_libraries(indexer PRIVATE tarantino_indexer)

if(NOT TARANTINO_BUILD_TESTS)
    return()
endif()

add_executable(indexer_tests
    tests/IndexerFactoryTest.cpp
    tests/adapters/datalake/DatalakeReadersTest.cpp
    tests/adapters/index/PendingPostingsTest.cpp
    tests/adapters/index/folders/FolderPerTermIndexAdapterTest.cpp
    tests/adapters/index/folders/FoldersNamesConformanceTest.cpp
    tests/adapters/index/folders/TermFileRestore.cpp
    tests/adapters/index/folders/TermFileRestoreTest.cpp
    tests/adapters/index/folders/TermFilesTest.cpp
    tests/adapters/index/json/MonolithicJsonIndexAdapterTest.cpp
    tests/adapters/metadata/PortablePathsTest.cpp
    tests/adapters/metadata/SqliteMetadataAdapterTest.cpp
    tests/adapters/stopwords/FileStopwordsLoaderTest.cpp
    tests/commands/IndexBookCommandTest.cpp
    tests/conformance/IndexerConformanceTest.cpp
    tests/model/book/HeaderParserTest.cpp
    tests/model/terms/TokenizerEquivalenceTest.cpp
    tests/model/terms/TokenizerTest.cpp)
target_include_directories(indexer_tests PRIVATE ${PROJECT_SOURCE_DIR}/services)
target_link_libraries(indexer_tests PRIVATE
    tarantino_indexer tarantino_crawler_conformance tarantino_testing tarantino_icu ${TARANTINO_SQLITE_TARGET}
    GTest::gtest_main)

if(TARANTINO_WITH_MONGO)
    # A database of its own on a running MongoDB, shared with the tests of the query and control services
    add_library(tarantino_indexer_test_mongo STATIC tests/adapters/mongo/TemporaryMongoDatabase.cpp)
    target_include_directories(tarantino_indexer_test_mongo PUBLIC ${PROJECT_SOURCE_DIR}/services)
    target_link_libraries(tarantino_indexer_test_mongo PUBLIC tarantino_platform tarantino_platform_mongo tarantino_mongocxx)
    target_sources(indexer_tests PRIVATE
        tests/adapters/mongo/MongoDatabasesTest.cpp
        tests/adapters/mongo/MongodbAdaptersTest.cpp)
    target_link_libraries(indexer_tests PRIVATE tarantino_indexer_test_mongo)
endif()

gtest_discover_tests(indexer_tests DISCOVERY_MODE PRE_TEST NO_PRETTY_VALUES
                     PROPERTIES ENVIRONMENT "TARANTINO_WORKLOAD=${PROJECT_SOURCE_DIR}/workload")

# The support code of the index and metadata benchmarks, shared with the benchmarks of the query service (SPEC §11)
add_library(tarantino_indexer_benchmark_support STATIC
    tests/adapters/index/folders/TermFileRestore.cpp
    tests/benchmarking/support/BenchmarkStore.cpp
    tests/benchmarking/support/IndexFixture.cpp
    tests/benchmarking/support/PrebuiltIndexes.cpp)
target_include_directories(tarantino_indexer_benchmark_support PUBLIC ${PROJECT_SOURCE_DIR}/services)
target_link_libraries(tarantino_indexer_benchmark_support
    PUBLIC tarantino_indexer tarantino_crawler_benchmark_support
    PRIVATE nlohmann_json::nlohmann_json)
if(TARANTINO_WITH_MONGO)
    target_sources(tarantino_indexer_benchmark_support PRIVATE tests/benchmarking/support/MongoStores.cpp)
    target_link_libraries(tarantino_indexer_benchmark_support PUBLIC tarantino_mongocxx tarantino_platform_mongo)
endif()

add_executable(indexer_benchmarks
    tests/benchmarking/BenchmarkRunner.cpp
    tests/benchmarking/index/FullIndexBuildBenchmark.cpp
    tests/benchmarking/index/IncrementalUpdateBenchmark.cpp
    tests/benchmarking/metadata/MetadataInsertionBenchmark.cpp
    $<TARGET_OBJECTS:tarantino_allocation_counter>)
target_link_libraries(indexer_benchmarks PRIVATE
    tarantino_indexer_benchmark_support ${TARANTINO_SQLITE_TARGET} nlohmann_json::nlohmann_json)
