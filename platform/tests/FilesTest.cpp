#include "tarantino/platform/Files.hpp"

#include "tarantino/platform/Paths.hpp"
#include "tarantino/testing/TempDirectory.hpp"

#include <gtest/gtest.h>

#include <algorithm>

namespace tarantino::platform::files {
namespace {

class FilesTest : public ::testing::Test {
protected:
    testing::TempDirectory directory;
};

TEST_F(FilesTest, writesAndReadsUtf8TextWithoutChangingLineEndings) {
    const Path file = directory / "text.txt";
    writeString(file, "café\nline\r\nend");

    EXPECT_EQ("café\nline\r\nend", readString(file));
    EXPECT_EQ(15u, size(file));
}

TEST_F(FilesTest, refusesToReadMalformedUtf8AsText) {
    const Path file = directory / "latin1.txt";
    writeString(file, "caf\xE9");

    EXPECT_THROW(readString(file), std::filesystem::filesystem_error);
    EXPECT_EQ("caf\xE9", readBytes(file));
}

TEST_F(FilesTest, readsLinesEndedOnlyByLineFeeds) {
    const Path file = directory / "ids.txt";
    writeString(file, "5\n\n 84 \r\n13");

    EXPECT_EQ((std::vector<std::string>{"5", "", " 84 \r", "13"}), readLines(file));
}

TEST_F(FilesTest, appendsCreatingTheFile) {
    const Path file = directory / "control/downloaded_books.txt";
    createDirectories(file.parent_path());
    append(file, "1342\n");
    append(file, "84\n");

    EXPECT_EQ("1342\n84\n", readString(file));
}

TEST_F(FilesTest, movesOverAnExistingFile) {
    const Path target = directory / "index.json";
    writeString(target, "old");
    writeString(directory / "index.json.tmp", "new");

    move(directory / "index.json.tmp", target);

    EXPECT_EQ("new", readString(target));
    EXPECT_FALSE(files::exists(directory / "index.json.tmp"));
}

TEST_F(FilesTest, walksEachDirectoryBeforeItsEntries) {
    createDirectories(directory / "a/b");
    writeString(directory / "a/b/c.txt", "c");
    writeString(directory / "a/d.txt", "d");

    const std::vector<Path> paths = walk(directory.path());
    const auto position = [&](std::string_view relative) {
        return std::find(paths.begin(), paths.end(), directory / relative) - paths.begin();
    };

    EXPECT_EQ(5u, paths.size());
    EXPECT_EQ(directory.path(), paths.front());
    EXPECT_LT(position("a"), position("a/b"));
    EXPECT_LT(position("a/b"), position("a/b/c.txt"));
    EXPECT_LT(position("a"), position("a/d.txt"));
}

TEST_F(FilesTest, findsTheFirstMatchUpToAMaximumDepth) {
    createDirectories(directory / "20250925/14");
    writeString(directory / "20250925/14/5.body.txt", "body");
    const auto named = [](std::string_view name) {
        return [name](const Path& path) { return portable(path.filename()) == name; };
    };

    EXPECT_EQ(directory / "20250925/14/5.body.txt", find(directory.path(), 3, named("5.body.txt")));
    EXPECT_EQ(std::nullopt, find(directory.path(), 2, named("5.body.txt")));
    EXPECT_EQ(std::nullopt, find(directory.path(), 3, named("6.body.txt")));
}

TEST_F(FilesTest, listsTheEntriesOfADirectory) {
    createDirectories(directory / "1");
    writeString(directory / "2.txt", "");
    std::vector<Path> children = list(directory.path());
    std::sort(children.begin(), children.end());

    EXPECT_EQ((std::vector<Path>{directory / "1", directory / "2.txt"}), children);
    EXPECT_TRUE(isDirectory(directory / "1"));
    EXPECT_TRUE(isRegularFile(directory / "2.txt"));
    EXPECT_TRUE(isEmptyDirectory(directory / "1"));
}

TEST_F(FilesTest, namesTheEntriesOfADirectoryAsUtf8) {
    createDirectories(directory / "1");
    writeString(directory.path() / pathOf("café.txt"), "");
    std::vector<std::string> found = names(directory.path());
    std::sort(found.begin(), found.end());

    EXPECT_EQ((std::vector<std::string>{"1", "café.txt"}), found);
    EXPECT_TRUE(names(directory / "1").empty());
    EXPECT_TRUE(names(directory / "missing").empty());
    EXPECT_TRUE(names(directory.path() / pathOf("café.txt")).empty());
}

TEST_F(FilesTest, keepsModificationTimesAtTheFullPrecisionOfTheFileSystem) {
    const Path file = directory / "body.txt";
    writeString(file, "body");
    // A multiple of 100 ns, the precision of NTFS; ext4 and APFS keep nanoseconds
    const Instant time = *parseInstant("2025-09-25T15:29:00.1234567Z");

    setLastModified(file, time);

    EXPECT_EQ(time, lastModified(file));
}

TEST_F(FilesTest, readsTheModificationTimeOnlyOfAFileThatExists) {
    const Path file = directory / "body.txt";
    writeString(file, "body");
    const Instant time = *parseInstant("2025-09-25T15:29:00.1234567Z");
    setLastModified(file, time);

    EXPECT_EQ(std::optional<Instant>(time), lastModifiedIfExists(file));
    EXPECT_EQ(std::nullopt, lastModifiedIfExists(directory / "missing.txt"));
}

TEST_F(FilesTest, deletesFilesAndTrees) {
    createDirectories(directory / "tree/inside");
    writeString(directory / "tree/inside/file.txt", "x");
    writeString(directory / "single.txt", "x");

    files::remove(directory / "single.txt");
    removeAll(directory / "tree");

    EXPECT_FALSE(files::exists(directory / "single.txt"));
    EXPECT_FALSE(files::exists(directory / "tree"));
    EXPECT_FALSE(removeIfExists(directory / "single.txt"));
    EXPECT_THROW(files::remove(directory / "single.txt"), std::filesystem::filesystem_error);
}

TEST_F(FilesTest, copiesFiles) {
    writeString(directory / "source.txt", "content");

    files::copy(directory / "source.txt", directory / "target.txt");

    EXPECT_EQ("content", readString(directory / "target.txt"));
    EXPECT_THROW(files::copy(directory / "source.txt", directory / "target.txt"), std::filesystem::filesystem_error);
    files::copy(directory / "source.txt", directory / "target.txt", true);
}

TEST_F(FilesTest, handlesNamesThatWindowsReservesAndPathsLongerThan260Characters) {
    // Terms of the folders index such as con or aux are reserved device names on Windows (SPEC §12)
    const Path folder = directory / "c";
    createDirectories(folder);
    writeString(folder / "con.txt.tmp", "5\n");
    move(folder / "con.txt.tmp", folder / "con.txt");
    const Path longFile = directory / (std::string(120, 'a') + "/" + std::string(150, 'b') + ".txt");
    createDirectories(longFile.parent_path());
    writeString(longFile, "1342\n");

    EXPECT_EQ("5\n", readString(folder / "con.txt"));
    EXPECT_EQ((std::vector<Path>{folder / "con.txt"}), list(folder));
    EXPECT_EQ("1342\n", readString(longFile));
    EXPECT_TRUE(files::exists(longFile));
    removeAll(directory / std::string(120, 'a'));
    files::remove(folder / "con.txt");
    EXPECT_TRUE(isEmptyDirectory(folder));
}

TEST_F(FilesTest, readsTheBlockSizeOfTheFileSystem) {
    const std::uint64_t block = blockSize(directory.path());

    EXPECT_GE(block, 512u);
    EXPECT_EQ(0u, block & (block - 1));
}

}
}
