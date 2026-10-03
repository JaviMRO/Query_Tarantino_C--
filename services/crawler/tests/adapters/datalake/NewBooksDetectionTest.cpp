#include "tarantino/crawler/adapters/datalake/batch/BatchBasedDatalakeAdapter.hpp"
#include "tarantino/crawler/adapters/datalake/book/BookBasedDatalakeAdapter.hpp"
#include "tarantino/crawler/adapters/datalake/time/TimeBasedDatalakeAdapter.hpp"
#include "tarantino/platform/Files.hpp"
#include "tarantino/testing/TempDirectory.hpp"

#include <gtest/gtest.h>

namespace tarantino::crawler::adapters {
namespace {

using model::BookText;

const platform::Instant THRESHOLD = *platform::parseInstant("2025-09-25T15:30:00Z");

class NewBooksDetectionTest : public ::testing::Test {
protected:
    void saveAt(int bookId, std::string_view instant) {
        TimeBasedDatalakeAdapter(root.path(), platform::fixedClock(*platform::parseInstant(instant)))
                .save(BookText{bookId, "header", "body"});
    }

    static void assertOnlyRecentBooksDetected(ports::DatalakeStorage& datalake) {
        const std::filesystem::path oldBody = datalake.save(BookText{1, "header", "body"}).body;
        datalake.save(BookText{1500, "header", "body"});
        platform::files::setLastModified(oldBody, THRESHOLD - std::chrono::seconds(60));

        EXPECT_EQ(std::unordered_set<int>{1500}, datalake.idsStoredSince(THRESHOLD));
    }

    testing::TempDirectory root;
};

TEST_F(NewBooksDetectionTest, timeBasedLayoutSelectsHourDirectoriesFromTheHourOfTheInstant) {
    saveAt(1, "2025-09-25T14:59:00Z");
    saveAt(2, "2025-09-25T15:10:00Z");
    saveAt(3, "2025-09-26T09:00:00Z");

    EXPECT_EQ((std::unordered_set<int>{2, 3}), TimeBasedDatalakeAdapter(root.path()).idsStoredSince(THRESHOLD));
}

TEST_F(NewBooksDetectionTest, bookBasedLayoutSelectsBodiesModifiedSinceTheInstant) {
    BookBasedDatalakeAdapter datalake(root.path());
    assertOnlyRecentBooksDetected(datalake);
}

TEST_F(NewBooksDetectionTest, batchBasedLayoutSelectsBodiesModifiedSinceTheInstant) {
    BatchBasedDatalakeAdapter datalake(root.path());
    assertOnlyRecentBooksDetected(datalake);
}

TEST_F(NewBooksDetectionTest, emptyDatalakesDetectNothing) {
    EXPECT_TRUE(BookBasedDatalakeAdapter(root / "missing").idsStoredSince(THRESHOLD).empty());
    EXPECT_TRUE(TimeBasedDatalakeAdapter(root / "missing").idsStoredSince(THRESHOLD).empty());
}

TEST_F(NewBooksDetectionTest, comparesModificationTimesAtFullPrecision) {
    BookBasedDatalakeAdapter datalake(root.path());
    const std::filesystem::path body = datalake.save(BookText{7, "header", "body"}).body;
    platform::files::setLastModified(body, THRESHOLD - std::chrono::microseconds(1));

    EXPECT_TRUE(datalake.idsStoredSince(THRESHOLD).empty());
    platform::files::setLastModified(body, THRESHOLD);
    EXPECT_EQ(std::unordered_set<int>{7}, datalake.idsStoredSince(THRESHOLD));
}

}
}
