#pragma once

#include "tarantino/crawler/CrawlerConfig.hpp"
#include "tarantino/crawler/commands/IngestBookCommand.hpp"
#include "tarantino/crawler/ports/BookDownloader.hpp"
#include "tarantino/crawler/ports/DatalakeStorage.hpp"

#include <memory>
#include <ostream>
#include <vector>

namespace tarantino::crawler {

/// Builds the adapters of the configuration. An unknown value throws std::invalid_argument naming it (SPEC §2).
class CrawlerFactory {
public:
    CrawlerFactory() = delete;

    static commands::IngestBookCommand ingestCommand(const CrawlerConfig& config);

    /// Run once before ingesting, so that an interrupted previous run leaves nothing behind (SPEC §6); what it removes
    /// is reported on out.
    static int removeIncompleteWrites(const CrawlerConfig& config, std::ostream& out);

    static std::shared_ptr<ports::BookDownloader> downloader(const CrawlerConfig& config);

    /// Every book of the local mirror of the configuration, in ascending id order.
    static std::vector<int> mirrorBookIds(const CrawlerConfig& config);

    static std::shared_ptr<ports::DatalakeStorage> datalake(const CrawlerConfig& config);
};

}
