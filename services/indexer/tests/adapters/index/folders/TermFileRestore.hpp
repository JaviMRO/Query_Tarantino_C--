#pragma once

#include <filesystem>
#include <string>
#include <unordered_set>

namespace tarantino::indexer::adapters {

/// Puts back the term files of some terms as they are in a snapshot, deleting the ones it does not have. Files are
/// restored in parallel, and each folder is created once, before the first file copied into it.
class TermFileRestore {
public:
    TermFileRestore() = delete;

    static void restore(const std::filesystem::path& snapshot, const std::filesystem::path& index,
                        const std::unordered_set<std::string>& terms);
};

}
