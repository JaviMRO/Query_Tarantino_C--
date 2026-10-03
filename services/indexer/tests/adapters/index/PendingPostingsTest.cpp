#include "tarantino/indexer/adapters/index/PendingPostings.hpp"

#include <gtest/gtest.h>

namespace tarantino::indexer::adapters {
namespace {

using model::TermOccurrences;

TEST(PendingPostingsTest, areEmptiedWhenDrained) {
    PendingPostings pending;
    pending.add(TermOccurrences{5, {{"island", 1}}});
    pending.add(TermOccurrences{1342, {{"island", 2}}});

    EXPECT_EQ((std::map<std::string, std::set<int>>{{"island", {5, 1342}}}), pending.drain());
    EXPECT_TRUE(pending.drain().empty());
}

TEST(PendingPostingsTest, areDrainedInTermOrder) {
    PendingPostings pending;
    pending.add(TermOccurrences{5, {{"whale", 1}, {"island", 1}, {"écume", 1}, {"ahab", 1}}});

    std::vector<std::string> terms;
    for (const auto& [term, ids] : pending.drain()) {
        terms.push_back(term);
    }

    EXPECT_EQ((std::vector<std::string>{"ahab", "island", "whale", "écume"}), terms);
}

}
}
