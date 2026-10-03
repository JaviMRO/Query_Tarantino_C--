#pragma once

#include "tarantino/indexer/model/terms/TermOccurrences.hpp"
#include "tarantino/platform/Text.hpp"

#include <map>
#include <set>
#include <string>

namespace tarantino::indexer::adapters {

/// Postings added since the last flush, in a hash map from term to the sorted set of its new ids, drained in term
/// order (SPEC §8.1): a flush then writes the terms of each folder together and inserts into MongoDB's term index in
/// key order, and does so the same way on every run. Terms order by their UTF-8 bytes, which order like code points.
class PendingPostings {
public:
    void add(const model::TermOccurrences& occurrences);

    /// The postings, sorted by term, leaving none behind.
    std::map<std::string, std::set<int>> drain();

private:
    platform::StringMap<std::set<int>> postings_;
};

}
