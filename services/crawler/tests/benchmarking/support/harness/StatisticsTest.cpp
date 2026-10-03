#include "crawler/tests/benchmarking/support/harness/Statistics.hpp"

#include <gtest/gtest.h>

#include <cmath>

namespace tarantino::crawler::benchmarking {
namespace {

TEST(StatisticsTest, givesTheQuantilesOfStudentsTDistribution) {
    EXPECT_NEAR(12.706204736, Statistics::studentQuantile(0.975, 1), 1e-6);
    EXPECT_NEAR(4.302652730, Statistics::studentQuantile(0.975, 2), 1e-6);
    EXPECT_NEAR(2.570581836, Statistics::studentQuantile(0.975, 5), 1e-6);
    EXPECT_NEAR(2.262157163, Statistics::studentQuantile(0.975, 9), 1e-6);
    EXPECT_NEAR(1.983971519, Statistics::studentQuantile(0.975, 100), 1e-6);
    EXPECT_NEAR(-2.570581836, Statistics::studentQuantile(0.025, 5), 1e-6);
}

TEST(StatisticsTest, givesTheMeanAndTheHalfWidthOfIts95ConfidenceInterval) {
    const Statistics statistics({10.0, 12.0, 14.0});

    EXPECT_DOUBLE_EQ(12.0, statistics.mean());
    EXPECT_DOUBLE_EQ(2.0, statistics.standardDeviation());
    // t(0.975, 2) = 4.303: 4.303 × 2 / √3 = 4.968
    EXPECT_NEAR(4.968, statistics.meanErrorAt(0.95), 1e-3);
}

TEST(StatisticsTest, cannotComputeTheErrorOfASingleSample) {
    EXPECT_TRUE(std::isnan(Statistics({5.0}).meanErrorAt(0.95)));
}

TEST(StatisticsTest, takesTheNearestRankAsPercentile) {
    Statistics statistics;
    for (int value = 1; value <= 200; ++value) {
        statistics.add(value);
    }

    EXPECT_DOUBLE_EQ(198.0, statistics.percentile(99));
    EXPECT_DOUBLE_EQ(100.0, statistics.percentile(50));
    EXPECT_DOUBLE_EQ(7.0, Statistics({7.0}).percentile(99));
}

}
}
