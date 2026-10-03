#include "tarantino/crawler/adapters/datalake/batch/BatchBasedDatalakeAdapter.hpp"
#include "tarantino/crawler/adapters/datalake/book/BookBasedDatalakeAdapter.hpp"
#include "tarantino/platform/Files.hpp"
#include "tarantino/testing/TempDirectory.hpp"

#include <gtest/gtest.h>

namespace tarantino::crawler::adapters {
namespace {

using model::BookText;
using model::StoredPaths;

class DirectPathDatalakeAdaptersTest : public ::testing::Test {
protected:
    testing::TempDirectory root;
};

TEST_F(DirectPathDatalakeAdaptersTest, bookBasedLayoutUsesOneDirectoryPerBook) {
    const StoredPaths paths = BookBasedDatalakeAdapter(root.path()).save(BookText{1342, "header", "body"});

    EXPECT_EQ((StoredPaths{root / "1342/header.txt", root / "1342/body.txt"}), paths);
    EXPECT_EQ("body", platform::files::readString(paths.body));
}

TEST_F(DirectPathDatalakeAdaptersTest, batchBasedLayoutGroupsBooksByThousands) {
    const StoredPaths paths = BatchBasedDatalakeAdapter(root.path()).save(BookText{64317, "header", "body"});

    EXPECT_EQ((StoredPaths{root / "64/64317.header.txt", root / "64/64317.body.txt"}), paths);
}

TEST_F(DirectPathDatalakeAdaptersTest, findsPathsOfStoredBooksOnly) {
    BatchBasedDatalakeAdapter datalake(root.path());
    const StoredPaths stored = datalake.save(BookText{5, "header", "body"});

    EXPECT_EQ(stored, datalake.pathsOf(5));
    EXPECT_EQ(std::nullopt, datalake.pathsOf(6));
}

}
}
