#pragma once

#include <filesystem>
#include <string_view>

namespace tarantino::indexer::adapters {

/// Term file names that stay distinct on case-insensitive and normalization-insensitive file systems (APFS, NTFS):
/// ASCII lowercase letters are kept and every other UTF-8 byte is written as %XX, so no two terms share a name; names
/// longer than 200 characters become the SHA-256 of the term (SPEC §8.1). Private to the folders structure.
class TermFiles {
public:
    TermFiles() = delete;

    /// <root>/<first code point, encoded>/<name>.txt
    static std::filesystem::path file(const std::filesystem::path& root, std::string_view term);
};

}
