# Crawler: downloads books, splits header and body and stores them in the datalake (SPEC §4-§6)

add_library(tarantino_crawler STATIC
    src/CrawlerConfig.cpp
    src/CrawlerFactory.cpp
    src/adapters/datalake/BookFiles.cpp
    src/adapters/datalake/batch/BatchBasedDatalakeAdapter.cpp
    src/adapters/datalake/book/BookBasedDatalakeAdapter.cpp
    src/adapters/datalake/time/TimeBasedDatalakeAdapter.cpp
    src/adapters/gutenberg/GutenbergHttpDownloader.cpp
    src/adapters/gutenberg/LocalMirrorDownloader.cpp
    src/commands/IngestBookCommand.cpp
    src/model/book/GutenbergText.cpp)
target_include_directories(tarantino_crawler PUBLIC include PRIVATE src)
target_link_libraries(tarantino_crawler PUBLIC tarantino_platform PRIVATE ${TARANTINO_CURL_TARGET})

add_executable(crawler src/main.cpp)
target_link_libraries(crawler PRIVATE tarantino_crawler)

if(NOT TARANTINO_BUILD_TESTS)
    return()
endif()

# The conformance cases, shared with the tests of the indexer and the query service as Java shares its test-jar
add_library(tarantino_crawler_conformance STATIC tests/conformance/ConformanceCases.cpp)
target_include_directories(tarantino_crawler_conformance PUBLIC ${PROJECT_SOURCE_DIR}/services)
target_link_libraries(tarantino_crawler_conformance PUBLIC tarantino_platform nlohmann_json::nlohmann_json)
# Found by ctest too, which lists the tests without TARANTINO_WORKLOAD and from the build directory
target_compile_definitions(tarantino_crawler_conformance PRIVATE TARANTINO_SOURCE_WORKLOAD="${PROJECT_SOURCE_DIR}/workload")

# The support code of the benchmarks, shared with the benchmarks of the indexer and the query service as Java shares
# its test-jar; the harness does for C++ what JMH does for Java (SPEC §11)
add_library(tarantino_crawler_benchmark_support STATIC
    tests/benchmarking/support/datalake/DatalakeFixture.cpp
    tests/benchmarking/support/datalake/RecoveryScenario.cpp
    tests/benchmarking/support/dataset/BenchmarkDataset.cpp
    tests/benchmarking/support/environment/BenchmarkOptions.cpp
    tests/benchmarking/support/environment/BenchmarkPaths.cpp
    tests/benchmarking/support/environment/Heap.cpp
    tests/benchmarking/support/files/Directories.cpp
    tests/benchmarking/support/files/FootprintLog.cpp
    tests/benchmarking/support/files/ResultsFile.cpp
    tests/benchmarking/support/harness/Harness.cpp
    tests/benchmarking/support/harness/Processes.cpp
    tests/benchmarking/support/harness/Statistics.cpp
    tests/benchmarking/support/results/Metric.cpp
    tests/benchmarking/support/results/ResultRow.cpp
    tests/benchmarking/support/results/Results.cpp)
target_include_directories(tarantino_crawler_benchmark_support PUBLIC ${PROJECT_SOURCE_DIR}/services)
target_link_libraries(tarantino_crawler_benchmark_support PUBLIC tarantino_crawler tarantino_platform Threads::Threads)
if(WIN32)
    target_link_libraries(tarantino_crawler_benchmark_support PUBLIC advapi32)
endif()

# The global operator new that counts the bytes allocated (memory_allocated), linked into each benchmark executable
add_library(tarantino_allocation_counter OBJECT tests/benchmarking/support/environment/AllocationCounter.cpp)
target_include_directories(tarantino_allocation_counter PRIVATE ${PROJECT_SOURCE_DIR}/services)
target_link_libraries(tarantino_allocation_counter PRIVATE tarantino_options)

add_executable(crawler_benchmarks
    tests/benchmarking/BenchmarkRunner.cpp
    tests/benchmarking/datalake/DatalakeLookupBenchmark.cpp
    tests/benchmarking/datalake/DatalakeWriteBenchmark.cpp
    tests/benchmarking/datalake/NewBooksDetectionBenchmark.cpp
    $<TARGET_OBJECTS:tarantino_allocation_counter>)
target_link_libraries(crawler_benchmarks PRIVATE tarantino_crawler_benchmark_support ${TARANTINO_CURL_TARGET})

add_executable(crawler_tests
    tests/benchmarking/support/datalake/CrawlClockTest.cpp
    tests/benchmarking/support/datalake/DatalakeRecoveryTest.cpp
    tests/benchmarking/support/environment/BenchmarkOptionsTest.cpp
    tests/benchmarking/support/files/DirectoriesTest.cpp
    tests/benchmarking/support/files/FootprintLogTest.cpp
    tests/benchmarking/support/harness/StatisticsTest.cpp
    tests/benchmarking/support/results/MetricTest.cpp
    tests/benchmarking/support/results/ResultRowTest.cpp
    tests/CrawlerFactoryTest.cpp
    tests/adapters/datalake/DirectPathDatalakeAdaptersTest.cpp
    tests/adapters/datalake/IncompleteWritesTest.cpp
    tests/adapters/datalake/NewBooksDetectionTest.cpp
    tests/adapters/datalake/time/TimeBasedDatalakeAdapterTest.cpp
    tests/adapters/gutenberg/GutenbergHttpDownloaderTest.cpp
    tests/adapters/gutenberg/LocalMirrorDownloaderTest.cpp
    tests/adapters/gutenberg/TestHttpServer.cpp
    tests/commands/IngestBookCommandTest.cpp
    tests/conformance/CrawlerConformanceTest.cpp
    tests/model/book/GutenbergTextTest.cpp)
target_link_libraries(crawler_tests PRIVATE
    tarantino_crawler tarantino_crawler_conformance tarantino_crawler_benchmark_support tarantino_testing GTest::gtest_main)
if(WIN32)
    target_link_libraries(crawler_tests PRIVATE ws2_32)
endif()
gtest_discover_tests(crawler_tests DISCOVERY_MODE PRE_TEST NO_PRETTY_VALUES
                     PROPERTIES ENVIRONMENT "TARANTINO_WORKLOAD=${PROJECT_SOURCE_DIR}/workload")
