# Control: orchestrates crawler -> indexer and tracks progress in control files (SPEC §9), with the crawler and indexer
# libraries linked into its executable (SPEC §16)

add_library(tarantino_control STATIC
    src/ControlConfig.cpp
    src/ControlFactory.cpp
    src/adapters/FileControlStateStore.cpp
    src/adapters/LocalCrawler.cpp
    src/adapters/LocalIndexer.cpp
    src/commands/ControlPipeline.cpp
    src/model/NextStep.cpp)
target_include_directories(tarantino_control PUBLIC include PRIVATE src)
target_link_libraries(tarantino_control PUBLIC tarantino_crawler tarantino_indexer tarantino_platform)

add_executable(control src/main.cpp)
target_link_libraries(control PRIVATE tarantino_control)

if(NOT TARANTINO_BUILD_TESTS)
    return()
endif()

add_executable(control_tests
    tests/ControlConfigTest.cpp
    tests/ControlFactoryTest.cpp
    tests/EndToEndTest.cpp
    tests/adapters/FileControlStateStoreTest.cpp
    tests/adapters/LocalAdaptersTest.cpp
    tests/commands/ControlPipelineTest.cpp
    tests/model/StepReportTest.cpp)
target_include_directories(control_tests PRIVATE ${PROJECT_SOURCE_DIR}/services)
target_link_libraries(control_tests PRIVATE tarantino_control tarantino_query tarantino_testing GTest::gtest_main)
if(TARANTINO_WITH_MONGO)
    target_link_libraries(control_tests PRIVATE tarantino_indexer_test_mongo)
endif()
gtest_discover_tests(control_tests DISCOVERY_MODE PRE_TEST NO_PRETTY_VALUES
                     PROPERTIES ENVIRONMENT "TARANTINO_WORKLOAD=${PROJECT_SOURCE_DIR}/workload")
