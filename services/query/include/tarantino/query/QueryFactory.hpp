#pragma once

#include "tarantino/query/QueryConfig.hpp"
#include "tarantino/query/commands/SearchCommand.hpp"
#include "tarantino/query/ports/InvertedIndexReader.hpp"
#include "tarantino/query/ports/MetadataReader.hpp"

#include <memory>

namespace tarantino::query {

/// Builds the adapters of the configuration. An unknown value throws std::invalid_argument naming it (SPEC §2), and so
/// does mongo in a build without MongoDB support.
class QueryFactory {
public:
    QueryFactory() = delete;

    static commands::SearchCommand searchCommand(const QueryConfig& config);

    static std::shared_ptr<ports::InvertedIndexReader> invertedIndex(const QueryConfig& config);

    static std::shared_ptr<ports::MetadataReader> metadata(const QueryConfig& config);

    static platform::StringSet stopwords(const QueryConfig& config);

    /// Whether this build has the MongoDB adapters (CMake option TARANTINO_MONGO).
    static bool withMongo();
};

}
