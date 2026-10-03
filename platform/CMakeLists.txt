# What the Java services take from the JDK, for the four C++ services: UTF-8 text and String.strip, ICU's Unicode rules,
# files and paths that work on every operating system, java.time instants, SHA-256, the environment and a thread pool.

add_library(tarantino_platform STATIC
    src/Environment.cpp
    src/Files.cpp
    src/Paths.cpp
    src/Sha256.cpp
    src/Text.cpp
    src/ThreadPool.cpp
    src/Time.cpp
    src/Unicode.cpp
    src/Utf8.cpp)
target_include_directories(tarantino_platform PUBLIC include)
target_link_libraries(tarantino_platform
    PUBLIC tarantino_options Threads::Threads
    PRIVATE tarantino_icu)
if(WIN32)
    target_link_libraries(tarantino_platform PRIVATE shell32)
endif()

if(TARANTINO_WITH_MONGO)
    add_library(tarantino_platform_mongo STATIC src/MongoDriver.cpp)
    target_include_directories(tarantino_platform_mongo PUBLIC include)
    target_link_libraries(tarantino_platform_mongo PUBLIC tarantino_options tarantino_mongocxx)
endif()

if(TARANTINO_BUILD_TESTS)
    # What every test shares, as JUnit's @TempDir
    add_library(tarantino_testing INTERFACE)
    target_include_directories(tarantino_testing INTERFACE testing/include)
    target_link_libraries(tarantino_testing INTERFACE tarantino_platform GTest::gtest)

    add_executable(platform_tests
        tests/EnvironmentTest.cpp
        tests/FilesTest.cpp
        tests/PathsTest.cpp
        tests/Sha256Test.cpp
        tests/TextTest.cpp
        tests/ThreadPoolTest.cpp
        tests/TimeTest.cpp
        tests/UnicodeTest.cpp
        tests/Utf8Test.cpp)
    target_link_libraries(platform_tests PRIVATE tarantino_platform tarantino_testing GTest::gtest_main)
    gtest_discover_tests(platform_tests DISCOVERY_MODE PRE_TEST NO_PRETTY_VALUES)
endif()
