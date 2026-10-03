#include "tarantino/indexer/adapters/index/PendingPostings.hpp"

namespace tarantino::indexer::adapters {

void PendingPostings::add(const model::TermOccurrences& occurrences) {
    for (const auto& [term, frequency] : occurrences.frequencies) {
        postings_.try_emplace(term).first->second.insert(occurrences.bookId);
    }
}

std::map<std::string, std::set<int>> PendingPostings::drain() {
    std::map<std::string, std::set<int>> drained;
    while (!postings_.empty()) {
        auto node = postings_.extract(postings_.begin());
        drained.emplace(std::move(node.key()), std::move(node.mapped()));
    }
    return drained;
}

}
