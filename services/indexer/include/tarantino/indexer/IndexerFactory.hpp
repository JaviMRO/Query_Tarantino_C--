#pragma once

#include "tarantino/indexer/IndexerConfig.hpp"
#include "tarantino/indexer/commands/IndexBookCommand.hpp"
#include "tarantino/indexer/ports/datamarts/InvertedIndexStorage.hpp"
#include "tarantino/indexer/ports/datamarts/MetadataStorage.hpp"
#include "tarantino/indexer/ports/sources/DatalakeReader.hpp"

#include <memory>

namespace tarantino::indexer {

/// Builds the adapters of the configuration. An unknown value throws std::invalid_argument naming it (SPEC §2), and so
/// does mongo in a build without MongoDB support.
class IndexerFactory {
public:
    IndexerFactory() = delete;

    static commands::IndexBookCommand indexCommand(const IndexerConfig& config);

    static std::shared_ptr<ports::DatalakeReader> datalakeReader(const IndexerConfig& config);

    static std::shared_ptr<ports::InvertedIndexStorage> invertedIndex(const IndexerConfig& config);

    static std::shared_ptr<ports::MetadataStorage> metadata(const IndexerConfig& config);

    /// Whether this build has the MongoDB adapters (CMake option TARANTINO_MONGO).
    static bool withMongo();
};

}
