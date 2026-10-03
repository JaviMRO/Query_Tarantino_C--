#include "tarantino/crawler/CrawlerFactory.hpp"
#include "tarantino/platform/Files.hpp"
#include "tarantino/testing/TempDirectory.hpp"

#include <gtest/gtest.h>

#include <algorithm>

namespace tarantino::crawler::adapters {
namespace {

namespace files = platform::files;
using model::BookText;
using model::StoredPaths;

class IncompleteWritesTest : public ::testing::TestWithParam<std::string> {
protected:
    std::vector<std::filesystem::path> remainingFiles() const {
        std::vector<std::filesystem::path> remaining;
        files::walk(root.path(), 100, [&](const files::WalkedPath& walked) {
            if (walked.regularFile) {
                remaining.push_back(walked.path);
            }
            return true;
        });
        std::sort(remaining.begin(), remaining.end());
        return remaining;
    }

    testing::TempDirectory root;
};

TEST_P(IncompleteWritesTest, removesTemporaryFilesAndHeadersWithoutBodyButKeepsStoredBooks) {
    const auto datalake = CrawlerFactory::datalake(CrawlerConfig{root.path(), GetParam(), std::nullopt});
    const StoredPaths stored = datalake->save(BookText{1342, "header", "body"});
    const StoredPaths interrupted = datalake->save(BookText{2001, "header", "body"});
    std::filesystem::path temporaryBody = interrupted.body;
    temporaryBody += ".tmp";
    files::move(interrupted.body, temporaryBody);

    EXPECT_EQ(2, datalake->removeIncompleteWrites());
    std::vector<std::filesystem::path> expected{stored.body, stored.header};
    std::sort(expected.begin(), expected.end());
    EXPECT_EQ(expected, remainingFiles());
    EXPECT_TRUE(datalake->pathsOf(1342).has_value());
}

TEST_P(IncompleteWritesTest, removesNothingFromAnEmptyOrMissingDatalake) {
    EXPECT_EQ(0, CrawlerFactory::datalake(CrawlerConfig{root / "missing", GetParam(), std::nullopt})->removeIncompleteWrites());
}

INSTANTIATE_TEST_SUITE_P(Layouts, IncompleteWritesTest, ::testing::Values("time", "book", "batch"));

TEST(IncompleteWritesOfTheBookLayoutTest, removesTheDirectoriesLeftEmpty) {
    testing::TempDirectory root;
    const auto datalake = CrawlerFactory::datalake(CrawlerConfig{root.path(), "book", std::nullopt});
    const StoredPaths interrupted = datalake->save(BookText{2001, "header", "body"});
    files::remove(interrupted.body);

    datalake->removeIncompleteWrites();

    EXPECT_TRUE(files::list(root.path()).empty());
}

}
}
