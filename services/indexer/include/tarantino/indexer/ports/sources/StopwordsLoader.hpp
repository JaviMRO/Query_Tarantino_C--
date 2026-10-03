#pragma once

#include "tarantino/platform/Text.hpp"

namespace tarantino::indexer::ports {

/// The stopwords the tokenizer leaves out (SPEC §3, §7).
class StopwordsLoader {
public:
    virtual ~StopwordsLoader() = default;

    virtual platform::StringSet stopwords() = 0;
};

}
