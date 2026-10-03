#include "tarantino/indexer/adapters/stopwords/FileStopwordsLoader.hpp"
#include "tarantino/platform/Files.hpp"
#include "tarantino/testing/TempDirectory.hpp"

#include <gtest/gtest.h>

namespace tarantino::indexer::adapters {
namespace {

TEST(FileStopwordsLoaderTest, stripsAndLowercasesEntriesIgnoringBlankLines) {
    testing::TempDirectory directory;
    const std::filesystem::path file = directory / "stopwords.txt";
    platform::files::writeString(file, " The \n\nOF\r\nand\nÉTAIT\n");

    EXPECT_EQ((platform::StringSet{"the", "of", "and", "était"}), FileStopwordsLoader(file).stopwords());
}

}
}
