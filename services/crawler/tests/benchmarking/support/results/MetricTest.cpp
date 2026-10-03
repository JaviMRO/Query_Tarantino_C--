#include "crawler/tests/benchmarking/support/results/Metric.hpp"

#include <gtest/gtest.h>

#include <cmath>
#include <limits>

namespace tarantino::crawler::benchmarking {
namespace {

constexpr double PRECISION = 1e-9;

TEST(MetricTest, keepsMeasuredScoresAndErrorsAsTheyAre) {
    const ResultRow row = Metric::asMeasured("lookup_time", "\xC2\xB5s/op").row("book", 100, 0.43, 0.08);

    EXPECT_EQ((ResultRow{"book", "lookup_time", 100, 0.43, 0.08, "\xC2\xB5s/op"}), row);
}

TEST(MetricTest, turnsTheTimeToWriteNBooksIntoThroughputKeepingTheRelativeError) {
    const ResultRow row = Metric::booksPerSecond("write_throughput").row("batch", 100, 250, 25);

    EXPECT_NEAR(400, row.value, PRECISION);
    EXPECT_NEAR(40, row.error, PRECISION);
    EXPECT_EQ("books/s", row.unit);
}

TEST(MetricTest, dividesTheTimeOfARunByItsBooks) {
    const ResultRow row = Metric::perBook("incremental_update_time", 10).row("json", 2000, 2556, 120);

    EXPECT_NEAR(255.6, row.value, PRECISION);
    EXPECT_NEAR(12, row.error, PRECISION);
    EXPECT_EQ("ms/book", row.unit);
}

TEST(MetricTest, keepsAnUnknownErrorUnknown) {
    EXPECT_TRUE(std::isnan(Metric::asMeasured("query_time", "\xC2\xB5s/query")
                                   .row("json", 100, 2.9, std::numeric_limits<double>::quiet_NaN())
                                   .error));
}

}
}
