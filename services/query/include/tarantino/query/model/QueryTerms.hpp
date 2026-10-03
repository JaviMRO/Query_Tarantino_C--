#pragma once

#include "tarantino/platform/Text.hpp"

#include <string>
#include <string_view>
#include <vector>

namespace tarantino::query::model {

/// The terms of a query (SPEC §10): the tokenizer of SPEC §7 applied to it, without duplicates, in the order in which
/// they first appear.
class QueryTerms {
public:
    QueryTerms() = delete;

    static std::vector<std::string> of(std::string_view query, const platform::StringSet& stopwords);
};

}
