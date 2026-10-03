#include "crawler/tests/benchmarking/support/datalake/CrawlClock.hpp"
#include "crawler/tests/benchmarking/support/files/Directories.hpp"
#include "tarantino/crawler/adapters/datalake/time/TimeBasedDatalakeAdapter.hpp"
#include "tarantino/testing/TempDirectory.hpp"

#include <gtest/gtest.h>

namespace tarantino::crawler::benchmarking {
namespace {

const platform::Instant START = *platform::parseInstant("2025-09-25T23:00:00Z");

TEST(CrawlClockTest, advancesOneHourEveryHundredBooks) {
    EXPECT_EQ(START, CrawlClock::instantOf(START, 99));
    EXPECT_EQ(START + std::chrono::hours(1), CrawlClock::instantOf(START, 100));
    EXPECT_EQ(std::chrono::hours(3), CrawlClock::durationOf(250));
}

TEST(CrawlClockTest, spreadsTheTimeLayoutOverOneHourDirectoryPerHundredBooks) {
    testing::TempDirectory root;
    CrawlClock clock(START);
    adapters::TimeBasedDatalakeAdapter datalake(root.path(), clock.clock());
    for (int position = 0; position < 250; ++position) {
        clock.moveTo(position);
        datalake.save(model::BookText{position + 1, "header", "body"});
    }

    // 20250925/23, 20250926/00 and 20250926/01, plus their two day directories
    EXPECT_EQ(5u, Directories::footprint(root.path()).directories);
}

}
}
