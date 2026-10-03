#pragma once

#include "tarantino/query/model/BookMetadata.hpp"

#include <string>
#include <vector>

namespace tarantino::query::model {

/// The books that contain every term of a query, by ascending id (SPEC §10).
struct SearchResult {
    std::string query;
    std::vector<BookMetadata> books;

    bool operator==(const SearchResult&) const = default;
};

}
