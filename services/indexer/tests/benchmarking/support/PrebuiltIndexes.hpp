#pragma once

#include "indexer/tests/benchmarking/support/BenchmarkStore.hpp"
#include "indexer/tests/benchmarking/support/IndexFixture.hpp"

#include <filesystem>
#include <string>

namespace tarantino::indexer::benchmarking {

/// Indexes of the first N books, built once per benchmark run and shared by every process of the benchmarks that only
/// need them as a starting point. Their content is deterministic, so reusing them changes no result.
class PrebuiltIndexes {
public:
    PrebuiltIndexes() = delete;

    static BenchmarkStore of(const std::string& index, int books, IndexFixture& fixture);

    /// A copy of a prebuilt index for benchmarks that update it, copied once per run and shared by every process and
    /// benchmark method, which put it back to the prebuilt index before each run (SPEC §11) instead of copying it whole:
    /// for folders that is hundreds of thousands of files.
    static BenchmarkStore workingCopyOf(const BenchmarkStore& prebuilt, const std::string& index, int books);

    static std::filesystem::path file(const std::string& name);

    static void deleteAll();
};

}
