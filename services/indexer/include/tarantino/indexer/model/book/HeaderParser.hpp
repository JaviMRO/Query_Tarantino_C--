#pragma once

#include "tarantino/indexer/model/book/Book.hpp"
#include "tarantino/indexer/model/book/BookText.hpp"

#include <optional>
#include <string>
#include <string_view>

namespace tarantino::indexer::model {

/// Reads the metadata of a book from its header (SPEC §7).
class HeaderParser {
public:
    Book book(const BookText& text) const;

    /// The first line that starts with <name>: and has at least one character after the colon, as ^<name>:[ \t]*(.+)$
    /// matches in multiline mode where only \n ends a line: the rest of that line, stripped; empty if there is none.
    static std::optional<std::string> field(std::string_view header, std::string_view name);
};

}
