#include "tarantino/platform/Environment.hpp"

#include <gtest/gtest.h>

#include <cstdlib>

namespace tarantino::platform {
namespace {

void setVariable(const char* name, const char* value) {
#ifdef _WIN32
    _putenv_s(name, value);
#else
    setenv(name, value, 1);
#endif
}

TEST(EnvironmentTest, readsVariablesOrTheirDefaults) {
    setVariable("TARANTINO_ENVIRONMENT_TEST", "book");

    EXPECT_EQ("book", environmentVariable("TARANTINO_ENVIRONMENT_TEST"));
    EXPECT_EQ("book", environmentVariable("TARANTINO_ENVIRONMENT_TEST", "time"));
    EXPECT_EQ(std::nullopt, environmentVariable("TARANTINO_ENVIRONMENT_TEST_MISSING"));
    EXPECT_EQ("time", environmentVariable("TARANTINO_ENVIRONMENT_TEST_MISSING", "time"));
}

TEST(EnvironmentTest, readsTheArgumentsAfterTheProgramName) {
    char program[] = "query";
    char first[] = "adventure";
    char second[] = "island";
    char* argv[] = {program, first, second};

    const std::vector<std::string> arguments = commandLineArguments(3, argv);

#ifdef _WIN32
    // Windows reads them from the command line of this test process instead
    EXPECT_FALSE(arguments.size() > 0 && arguments.front() == "query");
#else
    EXPECT_EQ((std::vector<std::string>{"adventure", "island"}), arguments);
#endif
}

}
}
