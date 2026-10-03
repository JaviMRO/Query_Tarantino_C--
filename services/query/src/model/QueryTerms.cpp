#include "tarantino/query/model/QueryTerms.hpp"

#include "tarantino/platform/Unicode.hpp"
#include "tarantino/platform/Utf8.hpp"

namespace tarantino::query::model {

namespace {

constexpr std::size_t MIN_TERM_LENGTH = 2;

}

std::vector<std::string> QueryTerms::of(std::string_view query, const platform::StringSet& stopwords) {
    const std::string text = platform::unicode::toLowerCase(query);
    const std::string_view lowered(text);
    std::vector<std::string> terms;
    platform::StringSet seen;
    std::size_t index = 0;
    while (index < lowered.size()) {
        // A maximal run of letters, \p{L}+, and how many code points it has
        const std::size_t start = index;
        std::size_t codePoints = 0;
        std::size_t after = index;
        while (index < lowered.size()) {
            after = index;
            if (!platform::unicode::isLetter(platform::utf8::next(lowered, after))) {
                break;
            }
            index = after;
            ++codePoints;
        }
        if (codePoints >= MIN_TERM_LENGTH) {
            const std::string_view term = lowered.substr(start, index - start);
            if (!stopwords.contains(term) && seen.emplace(term).second) {
                terms.emplace_back(term);
            }
        }
        if (codePoints == 0) {
            index = after;
        }
    }
    return terms;
}

}
