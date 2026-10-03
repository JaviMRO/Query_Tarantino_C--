#pragma once

#include "tarantino/crawler/model/book/StoredPaths.hpp"
#include "tarantino/crawler/model/failure/FailureReason.hpp"

#include <optional>

namespace tarantino::crawler::commands {

/// Where a book was stored, or why it could not be.
struct IngestResult {
    int bookId;
    std::optional<model::StoredPaths> paths;
    std::optional<model::FailureReason> failure;

    static IngestResult success(int bookId, model::StoredPaths paths) {
        return {bookId, std::move(paths), std::nullopt};
    }

    static IngestResult failed(int bookId, model::FailureReason reason) {
        return {bookId, std::nullopt, reason};
    }

    bool succeeded() const {
        return !failure.has_value();
    }

    bool operator==(const IngestResult&) const = default;
};

}
