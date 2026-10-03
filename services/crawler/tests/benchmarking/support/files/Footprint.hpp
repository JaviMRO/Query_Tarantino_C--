#pragma once

#include <cstdint>
#include <ostream>

namespace tarantino::crawler::benchmarking {

/// What a directory tree takes: regular files, directories below the root, the sum of file sizes, and that sum with
/// each file rounded up to whole blocks of its file system (SPEC §11, disk_allocated).
struct Footprint {
    std::uint64_t files = 0;
    std::uint64_t directories = 0;
    std::uint64_t bytes = 0;
    std::uint64_t allocatedBytes = 0;

    bool operator==(const Footprint&) const = default;
};

inline std::ostream& operator<<(std::ostream& out, const Footprint& footprint) {
    return out << footprint.files << " files, " << footprint.directories << " directories, " << footprint.bytes
               << " bytes, " << footprint.allocatedBytes << " allocated";
}

}
