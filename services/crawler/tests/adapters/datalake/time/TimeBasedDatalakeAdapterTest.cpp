#include "tarantino/crawler/adapters/datalake/time/TimeBasedDatalakeAdapter.hpp"
#include "tarantino/platform/Files.hpp"
#include "tarantino/platform/Paths.hpp"
#include "tarantino/testing/TempDirectory.hpp"

#include <gtest/gtest.h>

namespace tarantino::crawler::adapters {
namespace {

namespace files = platform::files;
using model::BookText;
using model::StoredPaths;

class TimeBasedDatalakeAdapterTest : public ::testing::Test {
protected:
    testing::TempDirectory root;
    const platform::Clock clock = platform::fixedClock(*platform::parseInstant("2025-09-25T14:30:00Z"));
};

TEST_F(TimeBasedDatalakeAdapterTest, storesBooksUnderDayAndHourDirectories) {
    const StoredPaths paths = TimeBasedDatalakeAdapter(root.path(), clock).save(BookText{5, "header", "body"});

    EXPECT_EQ(root / "20250925/14/5.header.txt", paths.header);
    EXPECT_EQ(root / "20250925/14/5.body.txt", paths.body);
    EXPECT_EQ("header", files::readString(paths.header));
    EXPECT_EQ("body", files::readString(paths.body));
}

TEST_F(TimeBasedDatalakeAdapterTest, findsPathsOfStoredBooksOnly) {
    TimeBasedDatalakeAdapter datalake(root.path(), clock);
    const StoredPaths stored = datalake.save(BookText{5, "header", "body"});

    EXPECT_EQ(stored, datalake.pathsOf(5));
    EXPECT_EQ(std::nullopt, datalake.pathsOf(6));
}

TEST_F(TimeBasedDatalakeAdapterTest, findsBooksSavedInEarlierHours) {
    const StoredPaths earlier = TimeBasedDatalakeAdapter(root.path(), clock).save(BookText{5, "header", "body"});
    const platform::Clock later = [this] { return clock() + std::chrono::hours(24); };

    EXPECT_EQ(earlier, TimeBasedDatalakeAdapter(root.path(), later).pathsOf(5));
}

TEST_F(TimeBasedDatalakeAdapterTest, leavesNoTemporaryFilesBehind) {
    TimeBasedDatalakeAdapter(root.path(), clock).save(BookText{5, "header", "body"});

    for (const std::filesystem::path& path : files::walk(root.path())) {
        EXPECT_FALSE(platform::portable(path).ends_with(".tmp")) << platform::portable(path);
    }
}

}
}
