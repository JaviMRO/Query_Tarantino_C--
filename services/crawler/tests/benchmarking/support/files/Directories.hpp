#pragma once

#include "crawler/tests/benchmarking/support/files/Footprint.hpp"

#include <cstdint>
#include <filesystem>
#include <functional>

namespace tarantino::crawler::benchmarking {

/// Directory trees as the benchmarks handle them, with the many small files of a folders index in mind.
class Directories {
public:
    Directories() = delete;

    /// Deletes the files in parallel, as a folders index holds hundreds of thousands, and then the directories.
    static void remove(const std::filesystem::path& root);

    /// Creates the directories and then copies the files in parallel.
    static void copy(const std::filesystem::path& source, const std::filesystem::path& target);

    static std::uint64_t fileCount(const std::filesystem::path& root,
                                   const std::function<bool(const std::filesystem::path&)>& filter = nullptr);

    /// Measures the tree in one walk (SPEC §11): nothing for a missing directory.
    static Footprint footprint(const std::filesystem::path& root);
};

}
