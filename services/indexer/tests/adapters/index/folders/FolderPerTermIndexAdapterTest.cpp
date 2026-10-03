#include "tarantino/indexer/adapters/index/folders/FolderPerTermIndexAdapter.hpp"
#include "tarantino/platform/Files.hpp"
#include "tarantino/testing/TempDirectory.hpp"

#include <gtest/gtest.h>

namespace tarantino::indexer::adapters {
namespace {

namespace files = platform::files;
using model::TermOccurrences;

class FolderPerTermIndexAdapterTest : public ::testing::Test {
protected:
    testing::TempDirectory root;
};

TEST_F(FolderPerTermIndexAdapterTest, mergesSortedUniquePostingsIntoOneFilePerTerm) {
    FolderPerTermIndexAdapter index(root.path());
    index.add(TermOccurrences{1342, {{"island", 3}, {"\u00e9cume", 1}}});
    index.flush();
    index.add(TermOccurrences{5, {{"island", 1}}});
    index.add(TermOccurrences{1342, {{"island", 3}}});
    index.flush();

    EXPECT_EQ("5\n1342\n", files::readString(root / "i/island.txt"));
    EXPECT_EQ("1342\n", files::readString(root / "%C3%A9/%C3%A9cume.txt"));
}

TEST_F(FolderPerTermIndexAdapterTest, keepsTermsApartThatCaseOrNormalizationInsensitiveFileSystemsWouldMerge) {
    const std::vector<std::string> terms{"shape", "\u017fhape", "hei\u00dft", "heisst", "\u03bb\u03cc\u03b3\u03bf\u03c2",
                                         "\u03bb\u03cc\u03b3\u03bf\u03c3", "caf\u00e9", "cafe\u0301"};
    FolderPerTermIndexAdapter index(root.path());
    for (std::size_t book = 0; book < terms.size(); ++book) {
        index.add(TermOccurrences{static_cast<int>(book) + 1, {{terms[book], 1}}});
    }
    index.flush();

    std::size_t termFiles = 0;
    files::walk(root.path(), 10, [&](const files::WalkedPath& walked) {
        termFiles += walked.regularFile ? 1 : 0;
        return true;
    });
    EXPECT_EQ(terms.size(), termFiles);
}

TEST_F(FolderPerTermIndexAdapterTest, storesTermsThatWindowsReservesAsDeviceNames) {
    // Terms such as con (Spanish, Italian) or aux (French) cannot be file names on Windows without \\?\ (SPEC §12)
    FolderPerTermIndexAdapter index(root.path());
    index.add(TermOccurrences{2000, {{"con", 2}, {"aux", 1}, {"nul", 1}, {"prn", 1}}});
    index.flush();

    EXPECT_EQ("2000\n", files::readString(root / "c/con.txt"));
    EXPECT_EQ("2000\n", files::readString(root / "a/aux.txt"));
    EXPECT_EQ("2000\n", files::readString(root / "n/nul.txt"));
    EXPECT_EQ("2000\n", files::readString(root / "p/prn.txt"));
}

TEST_F(FolderPerTermIndexAdapterTest, flushingNothingWritesNothing) {
    FolderPerTermIndexAdapter(root / "index").flush();

    EXPECT_FALSE(files::exists(root / "index"));
}

}
}
