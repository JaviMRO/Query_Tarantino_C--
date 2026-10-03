#include "crawler/tests/benchmarking/support/files/FootprintLog.hpp"
#include "tarantino/platform/Files.hpp"
#include "tarantino/testing/TempDirectory.hpp"

#include <gtest/gtest.h>

#include <cmath>

namespace tarantino::crawler::benchmarking {
namespace {

class FootprintLogTest : public ::testing::Test {
protected:
    testing::TempDirectory directory;
};

TEST_F(FootprintLogTest, keepsOneRowPerExactMeasureWhateverTheNumberOfProcesses) {
    const std::filesystem::path exact = directory / "exact.csv";
    for (int process = 0; process < 3; ++process) {
        FootprintLog::appendTo(exact, {ResultRow::exact("folders", "term_count", 100, 106'410, "terms")});
    }

    EXPECT_EQ(std::vector<ResultRow>{ResultRow::exact("folders", "term_count", 100, 106'410, "terms")},
              FootprintLog::drain(exact, directory / "samples.csv"));
    EXPECT_FALSE(platform::files::exists(exact));
}

TEST_F(FootprintLogTest, drainsSamplesAsTheirMeanAnd95ConfidenceHalfWidth) {
    const std::filesystem::path samples = directory / "samples.csv";
    for (const double value : {10.0, 12.0, 14.0}) {
        FootprintLog::appendTo(samples, {ResultRow::sample("json", "build_memory", 100, value, "bytes")});
    }

    const ResultRow mean = FootprintLog::drain(directory / "exact.csv", samples).front();

    // mean 12, standard deviation 2, t(0.975, 2) = 4.303: 4.303 × 2 / √3 = 4.968
    EXPECT_NEAR(12.0, mean.value, 1e-9);
    EXPECT_NEAR(4.968, mean.error, 1e-3);
}

TEST_F(FootprintLogTest, cannotComputeTheErrorOfASingleSample) {
    const std::filesystem::path samples = directory / "samples.csv";
    FootprintLog::appendTo(samples, {ResultRow::sample("json", "index_memory", 100, 49e6, "bytes")});

    EXPECT_TRUE(std::isnan(FootprintLog::drain(directory / "exact.csv", samples).front().error));
}

TEST_F(FootprintLogTest, drainsNothingWhenNothingWasAppended) {
    EXPECT_TRUE(FootprintLog::drain(directory / "exact.csv", directory / "samples.csv").empty());
}

}
}
