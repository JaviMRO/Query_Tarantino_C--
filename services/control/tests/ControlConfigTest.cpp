#include "tarantino/control/ControlConfig.hpp"

#include <gtest/gtest.h>

namespace tarantino::control {
namespace {

TEST(ControlConfigTest, readsPositiveIntegers) {
    EXPECT_EQ(100, ControlConfig::positiveInteger("TARANTINO_INDEX_BATCH", "100"));
    EXPECT_EQ(1, ControlConfig::positiveInteger("TARANTINO_PARALLEL_DOWNLOADS", " 1 "));
}

TEST(ControlConfigTest, rejectsValuesThatAreNotPositiveIntegersNamingTheVariable) {
    try {
        ControlConfig::positiveInteger("TARANTINO_PARALLEL_DOWNLOADS", "0");
        FAIL() << "0 was accepted";
    } catch (const std::invalid_argument& error) {
        EXPECT_STREQ("Unknown TARANTINO_PARALLEL_DOWNLOADS: 0 (expected a positive integer)", error.what());
    }
    EXPECT_THROW(ControlConfig::positiveInteger("TARANTINO_INDEX_BATCH", "many"), std::invalid_argument);
    EXPECT_THROW(ControlConfig::positiveInteger("TARANTINO_INDEX_BATCH", "-3"), std::invalid_argument);
}

}
}
