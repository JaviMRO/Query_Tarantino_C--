#include "crawler/tests/benchmarking/support/files/Directories.hpp"
#include "tarantino/platform/Files.hpp"
#include "tarantino/testing/TempDirectory.hpp"

#include <gtest/gtest.h>

namespace tarantino::crawler::benchmarking {
namespace {

namespace files = platform::files;

class DirectoriesTest : public ::testing::Test {
protected:
    testing::TempDirectory root;
};

TEST_F(DirectoriesTest, measuresFilesDirectoriesAndBytesRoundedUpToWholeBlocks) {
    const std::uint64_t block = files::blockSize(root.path());
    files::createDirectories(root / "a/b");
    files::writeString(root / "empty.txt", "");
    files::writeString(root / "a/one.txt", std::string(1, 'x'));
    files::writeString(root / "a/b/block.txt", std::string(block, 'x'));
    files::writeString(root / "a/b/more.txt", std::string(block + 1, 'x'));

    EXPECT_EQ((Footprint{4, 2, 2 * block + 2, 4 * block}), Directories::footprint(root.path()));
}

TEST_F(DirectoriesTest, measuresNothingForAMissingDirectory) {
    EXPECT_EQ(Footprint{}, Directories::footprint(root / "missing"));
}

TEST_F(DirectoriesTest, copiesAndDeletesWholeTrees) {
    files::createDirectories(root / "source/i");
    files::writeString(root / "source/i/island.txt", "5\n");
    files::writeString(root / "source/top.txt", "x");

    Directories::copy(root / "source", root / "target");
    Directories::remove(root / "source");

    EXPECT_EQ("5\n", files::readString(root / "target/i/island.txt"));
    EXPECT_EQ(2u, Directories::fileCount(root / "target"));
    EXPECT_FALSE(files::exists(root / "source"));
}

}
}
