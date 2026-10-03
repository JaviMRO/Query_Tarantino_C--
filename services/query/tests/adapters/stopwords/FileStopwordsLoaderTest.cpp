#include "tarantino/query/adapters/stopwords/FileStopwordsLoader.hpp"
#include "tarantino/platform/Files.hpp"
#include "tarantino/testing/TempDirectory.hpp"

#include <gtest/gtest.h>

namespace tarantino::query::adapters {
namespace {

TEST(FileStopwordsLoaderTest, stripsAndLowercasesEntriesIgnoringBlankLines) {
    testing::TempDirectory directory;
    const std::filesystem::path file = directory / "stopwords.txt";
    platform::files::writeString(file, " The \n\nOF\r\nand\n");

    EXPECT_EQ((platform::StringSet{"the", "of", "and"}), FileStopwordsLoader(file).stopwords());
}

}
}
