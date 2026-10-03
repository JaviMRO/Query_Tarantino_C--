#pragma once

#include "tarantino/crawler/model/book/BookText.hpp"
#include "tarantino/crawler/model/failure/FailureReason.hpp"

#include <optional>

namespace tarantino::crawler::commands {

/// A downloaded and split book, or why it could not be.
struct DownloadResult {
    int bookId;
    std::optional<model::BookText> text;
    std::optional<model::FailureReason> failure;

    static DownloadResult success(model::BookText text) {
        const int bookId = text.bookId;
        return {bookId, std::move(text), std::nullopt};
    }

    static DownloadResult failed(int bookId, model::FailureReason reason) {
        return {bookId, std::nullopt, reason};
    }

    bool succeeded() const {
        return !failure.has_value();
    }

    bool operator==(const DownloadResult&) const = default;
};

}
