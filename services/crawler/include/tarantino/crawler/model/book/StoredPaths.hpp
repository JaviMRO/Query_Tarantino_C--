#pragma once

#include <filesystem>

namespace tarantino::crawler::model {

/// Where the datalake stores the two files of a book (SPEC §6).
struct StoredPaths {
    std::filesystem::path header;
    std::filesystem::path body;

    bool operator==(const StoredPaths&) const = default;
};

}
