#include "tarantino/indexer/adapters/index/json/MonolithicJsonIndexAdapter.hpp"
#include "tarantino/platform/Files.hpp"
#include "tarantino/testing/TempDirectory.hpp"

#include <gtest/gtest.h>

namespace tarantino::indexer::adapters {
namespace {

namespace files = platform::files;
using model::TermOccurrences;

class MonolithicJsonIndexAdapterTest : public ::testing::Test {
protected:
    testing::TempDirectory directory;
};

TEST_F(MonolithicJsonIndexAdapterTest, writesSortedTermsWithSortedUniquePostings) {
    const std::filesystem::path file = directory / "datamarts/inverted_index.json";
    MonolithicJsonIndexAdapter index(file);
    index.add(TermOccurrences{1342, {{"whale", 1}, {"island", 2}}});
    index.add(TermOccurrences{5, {{"island", 1}}});
    index.add(TermOccurrences{5, {{"island", 1}}});
    index.flush();

    EXPECT_EQ(R"({"island":[5,1342],"whale":[1342]})", files::readString(file));
}

TEST_F(MonolithicJsonIndexAdapterTest, mergesNewBooksIntoAnExistingIndexFile) {
    const std::filesystem::path file = directory / "inverted_index.json";
    files::writeString(file, R"({"island":[5]})");

    MonolithicJsonIndexAdapter index(file);
    index.add(TermOccurrences{1342, {{"island", 1}}});
    index.flush();

    EXPECT_EQ(R"({"island":[5,1342]})", files::readString(file));
}

TEST_F(MonolithicJsonIndexAdapterTest, opensTheStoredIndexIntoMemoryBeforeAddingBooks) {
    const std::filesystem::path file = directory / "inverted_index.json";
    files::writeString(file, R"({"island":[5]})");
    MonolithicJsonIndexAdapter index(file);

    index.open();
    files::remove(file);
    index.add(TermOccurrences{1342, {{"whale", 1}}});
    index.flush();

    EXPECT_EQ(R"({"island":[5],"whale":[1342]})", files::readString(file));
}

TEST_F(MonolithicJsonIndexAdapterTest, readsAnyJsonWhitespaceAndWritesNonAsciiTermsAsUtf8) {
    const std::filesystem::path file = directory / "inverted_index.json";
    files::writeString(file, "{ \"écume\" : [ 7 , 3 ] ,\n \"c\\u00e6sar\": [1]\n}");

    MonolithicJsonIndexAdapter index(file);
    index.add(TermOccurrences{5, {{"écume", 1}}});
    index.flush();

    EXPECT_EQ("{\"cæsar\":[1],\"écume\":[3,5,7]}", files::readString(file));
}

TEST_F(MonolithicJsonIndexAdapterTest, refusesAFileThatIsNotAnIndex) {
    const std::filesystem::path file = directory / "inverted_index.json";
    files::writeString(file, R"({"island":["five"]})");

    EXPECT_THROW(MonolithicJsonIndexAdapter(file).open(), std::runtime_error);
}

}
}
