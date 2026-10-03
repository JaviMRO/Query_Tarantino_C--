#include "crawler/tests/benchmarking/support/datalake/RecoveryScenario.hpp"
#include "crawler/tests/ports/FunctionDownloader.hpp"
#include "tarantino/testing/TempDirectory.hpp"

#include <gtest/gtest.h>

#include <numeric>

namespace tarantino::crawler::benchmarking {
namespace {

const auto SYNTHETIC_BOOKS = ports::FunctionDownloader::of([](int bookId) {
    return "Title: Book " + std::to_string(bookId) + "\n*** START OF THE PROJECT GUTENBERG EBOOK X ***\nbody " + std::to_string(bookId)
           + "\n*** END OF THE PROJECT GUTENBERG EBOOK X ***";
});

std::vector<int> twentyBooks() {
    std::vector<int> ids(20);
    std::iota(ids.begin(), ids.end(), 1);
    return ids;
}

class DatalakeRecoveryTest : public ::testing::TestWithParam<std::string> {
protected:
    testing::TempDirectory root;
};

TEST_P(DatalakeRecoveryTest, resumesAfterInterruptionWithoutDuplicatesOrLosses) {
    EXPECT_TRUE(RecoveryScenario(GetParam(), root.path(), SYNTHETIC_BOOKS).run(twentyBooks()).recovered);
}

TEST_P(DatalakeRecoveryTest, leavesNoFilesBehindBecauseTheResumedRunRemovesIncompleteWrites) {
    EXPECT_EQ(0u, RecoveryScenario(GetParam(), root.path(), SYNTHETIC_BOOKS).run(twentyBooks()).leftoverFiles);
}

INSTANTIATE_TEST_SUITE_P(Layouts, DatalakeRecoveryTest, ::testing::Values("time", "book", "batch"));

}
}
