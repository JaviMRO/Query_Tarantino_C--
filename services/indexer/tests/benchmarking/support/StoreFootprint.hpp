#pragma once

#include <cstdint>

namespace tarantino::indexer::benchmarking {

/// Disk taken by an inverted index, logical and in whole blocks, and its number of distinct terms.
struct StoreFootprint {
    std::uint64_t bytes;
    std::uint64_t allocatedBytes;
    std::uint64_t terms;
};

}
