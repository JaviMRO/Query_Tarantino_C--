#include "tarantino/query/commands/SearchCommand.hpp"

#include "tarantino/query/model/QueryTerms.hpp"

namespace tarantino::query::commands {

SearchCommand::SearchCommand(std::shared_ptr<ports::InvertedIndexReader> invertedIndex,
                             std::shared_ptr<ports::MetadataReader> metadata, platform::StringSet stopwords)
    : invertedIndex_(std::move(invertedIndex)), metadata_(std::move(metadata)), stopwords_(std::move(stopwords)) {
}

model::SearchResult SearchCommand::execute(const std::string& query) {
    model::SearchResult result{query, {}};
    for (const int bookId : idsContainingAll(model::QueryTerms::of(query, stopwords_))) {
        if (auto book = metadata_->book(bookId)) {
            result.books.push_back(*std::move(book));
        }
    }
    return result;
}

std::set<int> SearchCommand::idsContainingAll(const std::vector<std::string>& terms) {
    std::set<int> result;
    bool first = true;
    for (const std::string& term : terms) {
        const ports::InvertedIndexReader::Postings postings = invertedIndex_->postings(term);
        std::set<int> ids(postings.ids().begin(), postings.ids().end());
        if (first) {
            result = std::move(ids);
            first = false;
        } else {
            std::erase_if(result, [&ids](int bookId) { return !ids.contains(bookId); });
        }
    }
    return result;
}

}
