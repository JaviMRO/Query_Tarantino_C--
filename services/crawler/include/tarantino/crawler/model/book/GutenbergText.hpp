#pragma once

#include "tarantino/crawler/model/book/BookText.hpp"

#include <string_view>

namespace tarantino::crawler::model {

/// Splits the raw text of a Project Gutenberg book into its header and body (SPEC §5).
class GutenbergText {
public:
    GutenbergText() = delete;

    /// The header before the start marker and the body between the markers, both stripped; the footer is discarded.
    /// Throws DownloadException with MISSING_MARKERS when either marker is missing.
    static BookText bookText(int bookId, std::string_view rawText);
};

}
