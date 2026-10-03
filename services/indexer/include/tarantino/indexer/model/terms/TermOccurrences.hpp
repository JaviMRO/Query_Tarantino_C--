#pragma once

#include "tarantino/platform/Text.hpp"

namespace tarantino::indexer::model {

/// The frequency of each term of a book (SPEC §7), counted in a hash map (Java's HashMap<String, Integer>).
struct TermOccurrences {
    int bookId;
    platform::StringMap<int> frequencies;

    bool operator==(const TermOccurrences&) const = default;
};

}
