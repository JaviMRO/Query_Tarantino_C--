#include "tarantino/control/adapters/LocalIndexer.hpp"

namespace tarantino::control::adapters {

using indexer::commands::IndexResult;

namespace {

model::Outcome outcome(const IndexResult& result) {
    if (result.indexed) {
        return model::Outcome::success(std::to_string(result.uniqueTerms) + " unique terms indexed");
    }
    return model::Outcome::failure("skipped, not found in the datalake");
}

}

LocalIndexer::LocalIndexer(indexer::commands::IndexBookCommand index) : index_(std::move(index)) {
}

std::map<int, model::Outcome> LocalIndexer::index(const std::vector<int>& bookIds) {
    std::map<int, model::Outcome> outcomes;
    for (const IndexResult& result : index_.execute(bookIds)) {
        outcomes.insert_or_assign(result.bookId, outcome(result));
    }
    return outcomes;
}

}
