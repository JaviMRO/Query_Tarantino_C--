#include "tarantino/query/commands/SearchCommand.hpp"

#include <gtest/gtest.h>

#include <map>

namespace tarantino::query::commands {
namespace {

using model::BookMetadata;
using model::SearchResult;

class MapIndex : public ports::InvertedIndexReader {
public:
    explicit MapIndex(std::map<std::string, std::unordered_set<int>> postings) : postings_(std::move(postings)) {
    }

    Postings postings(const std::string& term) override {
        lookups.push_back(term);
        const auto found = postings_.find(term);
        return found == postings_.end() ? Postings::none() : Postings::of(found->second);
    }

    std::vector<std::string> lookups;

private:
    std::map<std::string, std::unordered_set<int>> postings_;
};

class EveryBookButGhosts : public ports::MetadataReader {
public:
    std::optional<BookMetadata> book(int bookId) override {
        if (bookId == 999) {
            return std::nullopt;
        }
        return BookMetadata{bookId, "Book " + std::to_string(bookId), "Author", "English", std::to_string(bookId) + ".body.txt"};
    }

    std::vector<BookMetadata> booksBy(const std::string&) override {
        return {};
    }
};

class SearchCommandTest : public ::testing::Test {
protected:
    std::vector<int> ids(const std::string& query) {
        std::vector<int> found;
        for (const BookMetadata& book : search.execute(query).books) {
            found.push_back(book.bookId);
        }
        return found;
    }

    std::shared_ptr<MapIndex> index = std::make_shared<MapIndex>(std::map<std::string, std::unordered_set<int>>{
            {"island", {1342, 5, 84}}, {"whale", {2701, 84, 5}}, {"ghost", {999}}});
    SearchCommand search{index, std::make_shared<EveryBookButGhosts>(), platform::StringSet{"the"}};
};

TEST_F(SearchCommandTest, returnsBooksContainingEveryTermOrderedById) {
    EXPECT_EQ((std::vector<int>{5, 84}), ids("The WHALE island"));
}

TEST_F(SearchCommandTest, returnsNothingWhenATermIsUnknown) {
    EXPECT_TRUE(ids("island unicorn").empty());
}

TEST_F(SearchCommandTest, returnsNothingWhenOnlyStopwordsRemain) {
    EXPECT_TRUE(ids("the a").empty());
}

TEST_F(SearchCommandTest, dropsBooksWithoutMetadata) {
    EXPECT_TRUE(ids("ghost").empty());
}

TEST_F(SearchCommandTest, looksEveryTermUpEvenOnceTheResultIsEmpty) {
    ids("unicorn island whale");

    EXPECT_EQ((std::vector<std::string>{"unicorn", "island", "whale"}), index->lookups);
}

TEST_F(SearchCommandTest, keepsTheQueryAsWritten) {
    EXPECT_EQ("The WHALE", search.execute("The WHALE").query);
}

}
}
