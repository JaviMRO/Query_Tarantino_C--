#pragma once

#include <filesystem>
#include <string>

namespace tarantino::crawler::benchmarking {

/// Where the benchmarks find their dataset and write their files (SPEC §11).
class BenchmarkPaths {
public:
    BenchmarkPaths() = delete;

    /// TARANTINO_BENCHMARKS, benchmarks by default.
    static std::filesystem::path benchmarks();

    /// TARANTINO_WORKLOAD, workload by default.
    static std::filesystem::path workload();

    /// A file or directory of the scratch directory, which desktop indexers skip: macOS Spotlight skips directories
    /// whose name ends in .noindex; elsewhere the name is just a name.
    static std::filesystem::path scratch(const std::string& name);

    static std::filesystem::path scratchRoot();

    /// The MongoDB server of TARANTINO_MONGO_URI with the given database.
    static std::string mongoUri(const std::string& database);
};

}
