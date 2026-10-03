#include "tarantino/indexer/IndexerConfig.hpp"

#include "tarantino/platform/Environment.hpp"
#include "tarantino/platform/Paths.hpp"

namespace tarantino::indexer {

IndexerConfig IndexerConfig::fromEnvironment() {
    using platform::environmentVariable;
    return {platform::pathOf(environmentVariable("TARANTINO_DATALAKE", "datalake")),
            environmentVariable("TARANTINO_DATALAKE_LAYOUT", "time"),
            platform::pathOf(environmentVariable("TARANTINO_DATAMARTS", "datamarts")),
            environmentVariable("TARANTINO_INDEX", "json"),
            environmentVariable("TARANTINO_METADATA", "sqlite"),
            environmentVariable("TARANTINO_MONGO_URI", "mongodb://localhost:27017"),
            platform::pathOf(environmentVariable("TARANTINO_WORKLOAD", "workload"))};
}

}
