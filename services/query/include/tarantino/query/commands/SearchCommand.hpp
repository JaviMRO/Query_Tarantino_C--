#pragma once

#include "tarantino/platform/Text.hpp"
#include "tarantino/query/model/SearchResult.hpp"
#include "tarantino/query/ports/InvertedIndexReader.hpp"
#include "tarantino/query/ports/MetadataReader.hpp"

#include <memory>
#include <set>
#include <string>
#include <vector>

namespace tarantino::query::commands {

/// The search of SPEC §10: the books whose postings contain every term of the query (AND), computed as Java computes
/// it, then resolved one by one through the metadata.
class SearchCommand {
public:
    SearchCommand(std::shared_ptr<ports::InvertedIndexReader> invertedIndex, std::shared_ptr<ports::MetadataReader> metadata,
                  platform::StringSet stopwords);

    model::SearchResult execute(const std::string& query);

private:
    /// For each term, in query order, its postings copied into a sorted set (Java's TreeSet); the first set keeps only
    /// the ids present in each of the others (retainAll). Every term is looked up, even once the result is empty.
    std::set<int> idsContainingAll(const std::vector<std::string>& terms);

    std::shared_ptr<ports::InvertedIndexReader> invertedIndex_;
    std::shared_ptr<ports::MetadataReader> metadata_;
    platform::StringSet stopwords_;
};

}
