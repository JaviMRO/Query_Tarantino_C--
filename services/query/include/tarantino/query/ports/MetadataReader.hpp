#pragma once

#include "tarantino/query/model/BookMetadata.hpp"

#include <optional>
#include <string>
#include <vector>

namespace tarantino::query::ports {

/// Reads the metadata datamart of SPEC §8.2.
class MetadataReader {
public:
    virtual ~MetadataReader() = default;

    virtual std::optional<model::BookMetadata> book(int bookId) = 0;

    /// The books whose author contains the text, ignoring case (SPEC §10), by ascending id.
    virtual std::vector<model::BookMetadata> booksBy(const std::string& author) = 0;
};

}
