#pragma once

#include <string>

namespace tarantino::crawler::model {

/// A book split into the header and the body that the datalake stores (SPEC §5, §6).
struct BookText {
    int bookId;
    std::string header;
    std::string body;

    bool operator==(const BookText&) const = default;
};

}
