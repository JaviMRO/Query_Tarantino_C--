#pragma once

#include <filesystem>
#include <string>

namespace tarantino::indexer::model {

/// A book as the datalake holds it, and where its body is (SPEC §6).
struct BookText {
    int bookId;
    std::string header;
    std::string body;
    std::filesystem::path bodyPath;

    bool operator==(const BookText&) const = default;
};

}
