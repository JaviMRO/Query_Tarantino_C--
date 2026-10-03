#pragma once

#include <optional>
#include <string>

namespace tarantino::query::model {

/// The metadata of a book as the datamarts store it (SPEC §8.2): a missing field is empty, and the path of its body
/// keeps / as separator (SPEC §1).
struct BookMetadata {
    int bookId;
    std::optional<std::string> title;
    std::optional<std::string> author;
    std::optional<std::string> language;
    std::string path;

    bool operator==(const BookMetadata&) const = default;
};

}
