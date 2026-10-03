#include "tarantino/crawler/model/book/GutenbergText.hpp"

#include "tarantino/crawler/model/failure/DownloadException.hpp"
#include "tarantino/platform/Text.hpp"

#include <array>
#include <optional>
#include <string>

namespace tarantino::crawler::model {

namespace {

/// Where a marker starts and where its literal form ends.
struct Marker {
    std::size_t start;
    std::size_t end;
};

constexpr std::array<std::string_view, 2> ENDINGS{"E PROJECT GUTENBERG EBOOK", "IS PROJECT GUTENBERG EBOOK"};

/// The earliest marker that starts at or after from, as the first match of \*\*\* ?<WORD> OF (THE|THIS) PROJECT
/// GUTENBERG EBOOK: the earliest of its four literal forms (SPEC §5), found in one pass over "<WORD> OF TH". Two
/// occurrences of that key are at least its length apart, so the first one with a whole form has the earliest start.
std::optional<Marker> marker(std::string_view text, std::string_view word, std::size_t from) {
    const std::string key = std::string(word) + " OF TH";
    for (std::size_t at = text.find(key, from); at != std::string_view::npos; at = text.find(key, at + 1)) {
        std::optional<std::size_t> start;
        if (at >= from + 4 && text.substr(at - 4, 4) == "*** ") {
            start = at - 4;
        } else if (at >= from + 3 && text.substr(at - 3, 3) == "***") {
            start = at - 3;
        }
        if (!start) {
            continue;
        }
        const std::size_t afterKey = at + key.size();
        for (const std::string_view ending : ENDINGS) {
            if (text.substr(afterKey, ending.size()) == ending) {
                return Marker{*start, afterKey + ending.size()};
            }
        }
    }
    return std::nullopt;
}

DownloadException missingMarkers(int bookId) {
    return DownloadException(FailureReason::MissingMarkers, "Missing Gutenberg markers in book " + std::to_string(bookId));
}

}

BookText GutenbergText::bookText(int bookId, std::string_view rawText) {
    const std::string text = platform::replaceAll(rawText, "\r\n", "\n");
    const auto start = marker(text, "START", 0);
    // The start marker runs to the end of its line, which it includes, and is missing without one
    const std::size_t lineEnd = start ? text.find('\n', start->end) : std::string::npos;
    if (lineEnd == std::string::npos) {
        throw missingMarkers(bookId);
    }
    const std::size_t bodyStart = lineEnd + 1;
    const auto end = marker(text, "END", bodyStart);
    if (!end) {
        throw missingMarkers(bookId);
    }
    const std::string_view all(text);
    return BookText{bookId, std::string(platform::strip(all.substr(0, start->start))),
                    std::string(platform::strip(all.substr(bodyStart, end->start - bodyStart)))};
}

}
