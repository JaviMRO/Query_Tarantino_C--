#pragma once

#include "tarantino/platform/Text.hpp"
#include "tarantino/query/ports/InvertedIndexReader.hpp"

#include <filesystem>
#include <optional>
#include <unordered_set>

namespace tarantino::query::adapters {

/// Reads the json structure (SPEC §8.1): the whole file the first time a term is looked up, kept as a hash map from
/// term to a hash set of ids (Java's HashMap<String, HashSet<Integer>>); a missing file is an empty index.
class MonolithicJsonIndexReader : public ports::InvertedIndexReader {
public:
    explicit MonolithicJsonIndexReader(std::filesystem::path file);

    Postings postings(const std::string& term) override;

private:
    const platform::StringMap<std::unordered_set<int>>& index();

    std::filesystem::path file_;
    std::optional<platform::StringMap<std::unordered_set<int>>> index_;
};

}
