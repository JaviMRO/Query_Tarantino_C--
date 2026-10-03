#pragma once

#include "tarantino/indexer/model/book/BookText.hpp"

#include <filesystem>
#include <optional>

namespace tarantino::indexer::adapters {

/// Reads the two files of a book in any layout (SPEC §6).
class BookFiles {
public:
    BookFiles() = delete;

    /// The book, if its body exists: a book exists in the datalake if and only if its body file does.
    static std::optional<model::BookText> bookText(int bookId, const std::filesystem::path& header,
                                                   const std::filesystem::path& body);
};

}
