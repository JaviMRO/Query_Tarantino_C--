#pragma once

#include "tarantino/indexer/ports/datamarts/InvertedIndexStorage.hpp"

#include <filesystem>
#include <map>
#include <optional>
#include <set>
#include <string>

namespace tarantino::indexer::adapters {

/// The json structure (SPEC §8.1): one JSON object, term to array of ids, loaded whole when the index is opened or on
/// the first add and kept as a sorted map of sorted sets (Java's TreeMap<String, TreeSet<Integer>>); every flush
/// rewrites all of it, atomically.
class MonolithicJsonIndexAdapter : public ports::InvertedIndexStorage {
public:
    explicit MonolithicJsonIndexAdapter(std::filesystem::path file);

    void open() override;

    void add(const model::TermOccurrences& occurrences) override;

    void flush() override;

private:
    std::map<std::string, std::set<int>>& index();

    std::filesystem::path file_;
    std::optional<std::map<std::string, std::set<int>>> index_;
};

}
