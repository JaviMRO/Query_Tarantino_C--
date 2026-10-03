#include "tarantino/query/model/QueryTerms.hpp"

#include <gtest/gtest.h>

namespace tarantino::query::model {
namespace {

TEST(QueryTermsTest, normalizesLikeTheIndexerAndRemovesDuplicates) {
    EXPECT_EQ((std::vector<std::string>{"island", "shipwreck"}),
              QueryTerms::of("The ISLAND of a Shipwreck island", platform::StringSet{"the", "of"}));
}

TEST(QueryTermsTest, keepsNonAsciiLettersAndSplitsOnEverythingElse) {
    EXPECT_EQ((std::vector<std::string>{"cæsar", "façade", "rocín"}),
              QueryTerms::of("CÆSAR, façade & ROCÍN!", platform::StringSet{}));
}

TEST(QueryTermsTest, findsNoTermsInAnEmptyQuery) {
    EXPECT_TRUE(QueryTerms::of("", platform::StringSet{}).empty());
    EXPECT_TRUE(QueryTerms::of("a 1 -", platform::StringSet{}).empty());
}

}
}
