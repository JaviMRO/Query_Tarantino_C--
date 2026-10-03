#include "tarantino/query/commands/SearchCommand.hpp"
#include "tarantino/query/model/QueryTerms.hpp"

#include "crawler/tests/conformance/ConformanceCases.hpp"

#include <gtest/gtest.h>

#include <map>
#include <set>

namespace tarantino::query::conformance {
namespace {

using crawler::conformance::ConformanceCases;
using model::BookMetadata;

platform::StringSet stopwordsOf(const std::string& fileName) {
    const auto stopwords = ConformanceCases::stopwords(ConformanceCases::file(fileName));
    return platform::StringSet(stopwords.begin(), stopwords.end());
}

class EveryBook : public ports::MetadataReader {
public:
    std::optional<BookMetadata> book(int bookId) override {
        return BookMetadata{bookId, std::nullopt, std::nullopt, std::nullopt, std::to_string(bookId) + ".body.txt"};
    }

    std::vector<BookMetadata> booksBy(const std::string&) override {
        return {};
    }
};

/// The postings of the books of search.json, built with the same terms as queries.
class BooksIndex : public ports::InvertedIndexReader {
public:
    BooksIndex() {
        const nlohmann::json search = ConformanceCases::file("search.json");
        const platform::StringSet stopwords = stopwordsOf("search.json");
        for (const auto& [bookId, text] : search.at("books").items()) {
            for (const std::string& term : model::QueryTerms::of(text.get<std::string>(), stopwords)) {
                postings_[term].insert(std::stoi(bookId));
            }
        }
    }

    Postings postings(const std::string& term) override {
        const auto found = postings_.find(term);
        return found == postings_.end() ? Postings::none() : Postings::of(found->second);
    }

private:
    std::map<std::string, std::unordered_set<int>> postings_;
};

class QueryTermsConformanceTest : public ::testing::TestWithParam<nlohmann::json> {};

TEST_P(QueryTermsConformanceTest, turnsQueriesIntoTerms) {
    const nlohmann::json& testCase = GetParam();
    SCOPED_TRACE(testCase.at("name").get<std::string>());

    EXPECT_EQ(testCase.at("terms").get<std::vector<std::string>>(),
              model::QueryTerms::of(testCase.at("query").get<std::string>(), stopwordsOf("query_terms.json")));
}

INSTANTIATE_TEST_SUITE_P(Spec, QueryTermsConformanceTest, ::testing::ValuesIn(ConformanceCases::cases("query_terms.json")),
                         [](const auto& info) { return ConformanceCases::testName(info.index, info.param.at("name").template get<std::string>()); });

class SearchConformanceTest : public ::testing::TestWithParam<nlohmann::json> {};

TEST_P(SearchConformanceTest, findsTheBooksContainingEveryTerm) {
    const nlohmann::json& testCase = GetParam();
    const std::string query = testCase.at("query").get<std::string>();
    SCOPED_TRACE(query);
    commands::SearchCommand search(std::make_shared<BooksIndex>(), std::make_shared<EveryBook>(), stopwordsOf("search.json"));

    std::vector<int> found;
    for (const BookMetadata& book : search.execute(query).books) {
        found.push_back(book.bookId);
    }

    EXPECT_EQ(testCase.at("ids").get<std::vector<int>>(), found);
}

INSTANTIATE_TEST_SUITE_P(Spec, SearchConformanceTest, ::testing::ValuesIn(ConformanceCases::cases("search.json")),
                         [](const auto& info) { return ConformanceCases::testName(info.index, info.param.at("query").template get<std::string>()); });

}
}
