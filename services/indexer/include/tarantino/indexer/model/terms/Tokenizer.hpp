#pragma once

#include "tarantino/indexer/model/terms/TermOccurrences.hpp"
#include "tarantino/platform/Text.hpp"

#include <string_view>

namespace tarantino::indexer::model {

/// The terms of SPEC §7, found by scanning the code points of the lowercased text for runs of letters, as the Java
/// tokenizer does instead of matching \p{L}+. The whole text is lowercased at once, because lowercasing letter by
/// letter differs for the final sigma and for letters whose lowercase form is longer.
class Tokenizer {
public:
    explicit Tokenizer(platform::StringSet stopwords);

    TermOccurrences occurrences(int bookId, std::string_view body) const;

private:
    platform::StringSet stopwords_;
};

}
