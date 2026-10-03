#include "tarantino/query/adapters/index/json/MonolithicJsonIndexReader.hpp"
#include "tarantino/platform/Files.hpp"
#include "tarantino/testing/TempDirectory.hpp"

#include <gtest/gtest.h>

namespace tarantino::query::adapters {
namespace {

using Ids = std::unordered_set<int>;

class MonolithicJsonIndexReaderTest : public ::testing::Test {
protected:
    testing::TempDirectory directory;
};

TEST_F(MonolithicJsonIndexReaderTest, readsPostingsFromTheIndexFile) {
    const std::filesystem::path file = directory / "inverted_index.json";
    platform::files::writeString(file, R"({"island":[5,1342]})");

    MonolithicJsonIndexReader index(file);

    EXPECT_EQ((Ids{5, 1342}), index.postings("island").ids());
    EXPECT_TRUE(index.postings("whale").ids().empty());
}

TEST_F(MonolithicJsonIndexReaderTest, treatsAMissingIndexAsEmpty) {
    EXPECT_TRUE(MonolithicJsonIndexReader(directory / "missing.json").postings("island").ids().empty());
}

TEST_F(MonolithicJsonIndexReaderTest, keepsTheWholeIndexAfterTheFirstLookup) {
    const std::filesystem::path file = directory / "inverted_index.json";
    platform::files::writeString(file, R"({"island":[5],"whale":[84]})");
    MonolithicJsonIndexReader index(file);
    index.postings("island");

    platform::files::remove(file);

    EXPECT_EQ(Ids{84}, index.postings("whale").ids());
}

}
}
