#pragma once

#include "tarantino/crawler/model/book/BookText.hpp"
#include "tarantino/crawler/model/book/StoredPaths.hpp"
#include "tarantino/platform/Time.hpp"

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace tarantino::crawler::adapters {

/// The files of a book, as every layout stores them (SPEC §6): <prefix>header.txt and <prefix>body.txt, each written
/// to a .tmp file first and then moved over its name, the header before the body.
class BookFiles {
public:
    BookFiles() = delete;

    static model::StoredPaths write(const model::StoredPaths& paths, const model::BookText& book);

    /// The paths, if their body exists: the book is stored.
    static std::optional<model::StoredPaths> existing(model::StoredPaths paths);

    /// The names of the entries of the directory, as UTF-8, or none when it is not one.
    static std::vector<std::string> names(const std::filesystem::path& directory);

    /// Whether the file exists and was last modified at or after the instant, at full precision (SPEC §6).
    static bool modifiedSince(const std::filesystem::path& file, platform::Instant instant);

    /// The id that names a book file, its name without the suffix; empty when the name is not a book's.
    static std::optional<int> bookId(std::string_view name, std::string_view suffix);

    /// Removes every .tmp file, every header without its body next to it and every directory left empty under the
    /// root (SPEC §6), and returns how many files were removed.
    static int removeIncompleteWrites(const std::filesystem::path& root);

    /// The text of a directory under which names are joined to it as operator/ would join them: the directory and a
    /// separator when it needs one. Paths built from text are made once, instead of once per component joined.
    static std::filesystem::path::string_type prefix(const std::filesystem::path& directory);

    /// UTF-8 text, such as a book id or a file name, as the text of a path.
    static std::filesystem::path::string_type text(std::string_view utf8);
};

}
