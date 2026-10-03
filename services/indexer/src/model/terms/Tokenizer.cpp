#include "tarantino/indexer/model/terms/Tokenizer.hpp"

#include "tarantino/platform/Unicode.hpp"
#include "tarantino/platform/Utf8.hpp"

#include <string>

namespace tarantino::indexer::model {

namespace {

constexpr std::size_t MIN_TERM_LENGTH = 2;

/// The code point at index, whose end it moves index to: ASCII directly, the rest through the UTF-8 decoder.
inline char32_t nextCodePoint(std::string_view text, std::size_t& index) {
    const auto byte = static_cast<unsigned char>(text[index]);
    if (byte < 0x80) {
        ++index;
        return byte;
    }
    return platform::utf8::next(text, index);
}

}

Tokenizer::Tokenizer(platform::StringSet stopwords) : stopwords_(std::move(stopwords)) {
}

TermOccurrences Tokenizer::occurrences(int bookId, std::string_view body) const {
    const std::string text = platform::unicode::toLowerCase(body);
    const std::string_view lowered(text);
    TermOccurrences occurrences{bookId, {}};
    platform::StringMap<int>& counts = occurrences.frequencies;
    std::size_t index = 0;
    while (index < lowered.size()) {
        const std::size_t start = index;
        std::size_t codePoints = 0;
        std::size_t after = index;
        while (index < lowered.size()) {
            after = index;
            if (!platform::unicode::isLetter(nextCodePoint(lowered, after))) {
                break;
            }
            index = after;
            ++codePoints;
        }
        if (codePoints >= MIN_TERM_LENGTH) {
            const std::string_view term = lowered.substr(start, index - start);
            if (!stopwords_.contains(term)) {
                const auto counted = counts.find(term);
                if (counted == counts.end()) {
                    counts.emplace(term, 1);
                } else {
                    ++counted->second;
                }
            }
        }
        if (codePoints == 0) {
            // Past the code point that is not a letter
            index = after;
        }
    }
    return occurrences;
}

}
