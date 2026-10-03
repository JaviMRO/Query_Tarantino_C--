#include "tarantino/platform/Paths.hpp"

#include <gtest/gtest.h>

namespace tarantino::platform {
namespace {

TEST(PathsTest, writesPathsWithForwardSlashesOnEveryOperatingSystem) {
    const std::filesystem::path path = std::filesystem::path("datalake") / "20250925" / "14" / "5.body.txt";

    EXPECT_EQ("datalake/20250925/14/5.body.txt", portable(path));
}

TEST(PathsTest, givesTheNameOfTheLastComponentAsFilenameDoes) {
    for (const char* text : {"datalake/café/12.body.txt", "12.body.txt", "datalake/漢", "datalake/", ""}) {
        const std::filesystem::path path = pathOf(text);
        EXPECT_EQ(portable(path.filename()), fileName(path)) << text;
    }
    EXPECT_EQ("café", fileName(pathOf("datalake") / pathOf("café")));
}

TEST(PathsTest, readsAndWritesPathsAsUtf8) {
    EXPECT_EQ("datalake/café/漢.txt", portable(pathOf("datalake/café/漢.txt")));
    EXPECT_EQ(pathOf("a") / "b", pathOf("a/b"));
}

TEST(PathsTest, givesTheOperatingSystemPathsWithoutLengthLimitsOrReservedNames) {
    const std::filesystem::path relative = pathOf("datamarts/inverted_index/c/con.txt");
#ifdef _WIN32
    const std::wstring system = systemPath(relative).native();
    EXPECT_TRUE(system.starts_with(L"\\\\?\\")) << portable(systemPath(relative));
    EXPECT_TRUE(system.ends_with(L"\\datamarts\\inverted_index\\c\\con.txt"));
#else
    EXPECT_EQ(relative, systemPath(relative));
#endif
}

#ifdef _WIN32
TEST(PathsTest, givesWindowsNormalizedPathsWithBackslashesOnly) {
    EXPECT_EQ(L"\\\\?\\C:\\data\\marts\\x.txt", systemPath(pathOf("C:/data/marts/x.txt")).native());
    EXPECT_EQ(L"\\\\?\\C:\\data\\x.txt", systemPath(pathOf("C:/data/marts/../x.txt")).native());
    EXPECT_EQ(L"\\\\?\\C:\\data\\x.txt", systemPath(pathOf("C:/data/./x.txt")).native());
    EXPECT_EQ(L"\\\\?\\C:\\data\\x.txt", systemPath(pathOf("C:\\data\\\\x.txt")).native());
    EXPECT_EQ(L"\\\\?\\UNC\\server\\share\\x.txt", systemPath(pathOf("//server/share/x.txt")).native());
    EXPECT_EQ(L"\\\\?\\C:\\data\\x.txt", systemPath(pathOf("\\\\?\\C:\\data\\x.txt")).native());
    EXPECT_EQ(systemPathText(pathOf("C:/data/x.txt")), systemPath(pathOf("C:/data/x.txt")).native());
}
#endif

}
}
