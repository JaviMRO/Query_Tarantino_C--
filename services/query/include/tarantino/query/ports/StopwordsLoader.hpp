#pragma once

#include "tarantino/platform/Text.hpp"

namespace tarantino::query::ports {

/// The stopwords left out of queries, the same the indexer leaves out of books (SPEC §3, §7).
class StopwordsLoader {
public:
    virtual ~StopwordsLoader() = default;

    virtual platform::StringSet stopwords() = 0;
};

}
