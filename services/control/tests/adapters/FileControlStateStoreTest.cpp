#include "tarantino/control/adapters/FileControlStateStore.hpp"
#include "tarantino/platform/Files.hpp"
#include "tarantino/testing/TempDirectory.hpp"

#include <gtest/gtest.h>

namespace tarantino::control::adapters {
namespace {

namespace files = platform::files;

class FileControlStateStoreTest : public ::testing::Test {
protected:
    testing::TempDirectory root;
};

TEST_F(FileControlStateStoreTest, startsEmptyWhenNoControlFilesExist) {
    FileControlStateStore state(root / "control");

    EXPECT_TRUE(state.downloaded().empty());
    EXPECT_TRUE(state.indexed().empty());
}

TEST_F(FileControlStateStoreTest, appendsIdsWithLfKeepingFileOrder) {
    FileControlStateStore state(root / "control");
    state.markDownloaded(1342);
    state.markDownloaded(84);
    state.markIndexed(84);

    EXPECT_EQ((std::vector<int>{1342, 84}), state.downloaded());
    EXPECT_EQ(std::vector<int>{84}, state.indexed());
    EXPECT_EQ("1342\n84\n", files::readString(root / "control/downloaded_books.txt"));
}

TEST_F(FileControlStateStoreTest, ignoresBlankAndPartiallyWrittenLines) {
    files::writeString(root / "downloaded_books.txt", "5\n\n 84 \n13x");

    EXPECT_EQ((std::vector<int>{5, 84}), FileControlStateStore(root.path()).downloaded());
}

TEST_F(FileControlStateStoreTest, readsEachIdOnce) {
    files::writeString(root / "indexed_books.txt", "5\n84\n5\n");

    EXPECT_EQ((std::vector<int>{5, 84}), FileControlStateStore(root.path()).indexed());
}

}
}
