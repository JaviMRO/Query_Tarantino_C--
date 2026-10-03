#pragma once

#include <filesystem>
#include <string>

namespace tarantino::indexer {

/// The configuration of the indexer (SPEC §2).
struct IndexerConfig {
    std::filesystem::path datalake;
    std::string datalakeLayout;
    std::filesystem::path datamarts;
    std::string index;
    std::string metadata;
    std::string mongoUri;
    std::filesystem::path workload;

    static IndexerConfig fromEnvironment();
};

}
