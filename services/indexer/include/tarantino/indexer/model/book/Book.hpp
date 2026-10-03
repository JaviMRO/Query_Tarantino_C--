#pragma once

#include <filesystem>
#include <optional>
#include <string>

namespace tarantino::indexer::model {

/// The metadata of a book (SPEC §8.2): a missing field is empty, which the datamarts store as null.
struct Book {
    int bookId;
    std::optional<std::string> title;
    std::optional<std::string> author;
    std::optional<std::string> language;
    std::filesystem::path path;

    bool operator==(const Book&) const = default;
};

}
