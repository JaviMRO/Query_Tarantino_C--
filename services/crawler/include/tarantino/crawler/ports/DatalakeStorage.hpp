#pragma once

#include "tarantino/crawler/model/book/BookText.hpp"
#include "tarantino/crawler/model/book/StoredPaths.hpp"
#include "tarantino/platform/Time.hpp"

#include <optional>
#include <unordered_set>

namespace tarantino::crawler::ports {

/// The datalake of SPEC §6, in one of its layouts. Failures to write throw std::filesystem::filesystem_error.
class DatalakeStorage {
public:
    virtual ~DatalakeStorage() = default;

    virtual model::StoredPaths save(const model::BookText& book) = 0;

    /// The paths of a stored book: a book exists if and only if its body file exists.
    virtual std::optional<model::StoredPaths> pathsOf(int bookId) = 0;

    virtual std::unordered_set<int> idsStoredSince(platform::Instant instant) = 0;

    /// Removes what an interrupted run left behind (SPEC §6) and returns how many files were removed.
    virtual int removeIncompleteWrites() = 0;
};

}
