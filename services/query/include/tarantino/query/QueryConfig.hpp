#pragma once

#include <filesystem>
#include <string>

namespace tarantino::query {

/// The configuration of the query service (SPEC §2).
struct QueryConfig {
    std::filesystem::path datamarts;
    std::string index;
    std::string metadata;
    std::string mongoUri;
    std::filesystem::path workload;

    static QueryConfig fromEnvironment();
};

}
