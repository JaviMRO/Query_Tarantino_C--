#include "tarantino/indexer/model/terms/Tokenizer.hpp"
#include "tarantino/platform/Unicode.hpp"
#include "tarantino/platform/Utf8.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <random>

#ifdef TARANTINO_WINDOWS_ICU
extern "C" std::int8_t u_charType(std::int32_t codePoint);
#else
#include <unicode/uchar.h>
#endif

namespace tarantino::indexer::model {
namespace {

using Frequencies = platform::StringMap<int>;

const platform::StringSet STOPWORDS{"the", "of", "σας"};
// Letters, digits, punctuation, combining marks, supplementary letters and characters whose lowercase form depends on
// context (final sigma) or changes length (dotted capital I)
const std::vector<std::string> PIECES{
        "the", "The", "OF", "island", "ISLAND", "café", "café", "don't", "1984year", "x", "a", "ab",
        "ΟΔΟΣ", "λόγος", "ΣΑΣ", "İstanbul",
        "Straße", "STRAẞE", "ſhape", "Cæsar", "漢字", "한국어",
        "مرحبا", "\U0001D49C", "\U0001D49C\U0001D49C", "e-mail", "rock-and-roll", "  ", "\n",
        "—", "...", "123", "_", " ", "Ǆemal", "ǅ"};

/// \p{L}+ found another way than the tokenizer: the general category of each code point of the whole lowercased text,
/// as ICU gives it (Lu, Ll, Lt, Lm and Lo are 1 to 5).
Frequencies reference(const std::string& body) {
    const std::string text = platform::unicode::toLowerCase(body);
    Frequencies frequencies;
    std::string term;
    std::size_t codePoints = 0;
    const auto endTerm = [&] {
        if (codePoints >= 2 && !STOPWORDS.contains(term)) {
            ++frequencies[term];
        }
        term.clear();
        codePoints = 0;
    };
    std::size_t index = 0;
    while (index < text.size()) {
        const std::size_t start = index;
        const char32_t codePoint = platform::utf8::next(text, index);
        const int category = u_charType(static_cast<std::int32_t>(codePoint));
        if (category >= 1 && category <= 5) {
            term.append(text, start, index - start);
            ++codePoints;
        } else {
            endTerm();
        }
    }
    endTerm();
    return frequencies;
}

TEST(TokenizerEquivalenceTest, countsTheSameTermsAsTheLettersOfTheSpec) {
    const Tokenizer tokenizer(STOPWORDS);
    std::mt19937 random(20261001);
    for (int text = 0; text < 5000; ++text) {
        std::string body;
        for (int piece = static_cast<int>(random() % 12); piece >= 0; --piece) {
            body += PIECES[random() % PIECES.size()];
            body += random() % 2 == 0 ? " " : "";
        }
        ASSERT_EQ(reference(body), tokenizer.occurrences(1, body).frequencies) << body;
    }
}

}
}
