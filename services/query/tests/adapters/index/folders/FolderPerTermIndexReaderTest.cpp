#include "tarantino/query/adapters/index/folders/FolderPerTermIndexReader.hpp"
#include "tarantino/platform/Files.hpp"
#include "tarantino/testing/TempDirectory.hpp"

#include <gtest/gtest.h>

namespace tarantino::query::adapters {
namespace {

namespace files = platform::files;
using Ids = std::unordered_set<int>;

class FolderPerTermIndexReaderTest : public ::testing::Test {
protected:
    testing::TempDirectory root;
};

TEST_F(FolderPerTermIndexReaderTest, readsPostingsOfATermAndNothingForUnknownTerms) {
    files::createDirectories(root / "i");
    files::writeString(root / "i/island.txt", "5\n1342\n");

    FolderPerTermIndexReader index(root.path());

    EXPECT_EQ((Ids{5, 1342}), index.postings("island").ids());
    EXPECT_TRUE(index.postings("whale").ids().empty());
}

TEST_F(FolderPerTermIndexReaderTest, readsNonAsciiTermsFromTheirEncodedFileName) {
    files::createDirectories(root / "%C3%A9");
    files::writeString(root / "%C3%A9/%C3%A9cume.txt", "1342\n");

    EXPECT_EQ(Ids{1342}, FolderPerTermIndexReader(root.path()).postings("écume").ids());
}

TEST_F(FolderPerTermIndexReaderTest, readsTheFileAgainOnEveryLookup) {
    files::createDirectories(root / "i");
    files::writeString(root / "i/island.txt", "5\n");
    FolderPerTermIndexReader index(root.path());
    EXPECT_EQ(Ids{5}, index.postings("island").ids());

    files::writeString(root / "i/island.txt", "5\n84\n");

    EXPECT_EQ((Ids{5, 84}), index.postings("island").ids());
}

}
}
