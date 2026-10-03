#pragma once

#include "tarantino/query/ports/MetadataReader.hpp"

#include <mongocxx/collection.hpp>

namespace tarantino::query::adapters {

/// Reads the mongo metadata backend (SPEC §8.2): one find by book_id, or a find by an author regular expression with
/// every metacharacter escaped and the i option, which folds every letter (SPEC §12), sorted by book_id.
class MongodbMetadataReader : public ports::MetadataReader {
public:
    explicit MongodbMetadataReader(const std::string& connectionUri);

    std::optional<model::BookMetadata> book(int bookId) override;

    std::vector<model::BookMetadata> booksBy(const std::string& author) override;

    /// The text with a backslash before each regular expression metacharacter, \^$.|?*+()[]{}, so that it matches
    /// literally (SPEC §14).
    static std::string quoted(const std::string& text);

private:
    mongocxx::collection collection_;
};

}
