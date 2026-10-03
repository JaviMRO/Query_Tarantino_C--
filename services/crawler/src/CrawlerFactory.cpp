#include "tarantino/crawler/CrawlerFactory.hpp"

#include "tarantino/crawler/adapters/datalake/batch/BatchBasedDatalakeAdapter.hpp"
#include "tarantino/crawler/adapters/datalake/book/BookBasedDatalakeAdapter.hpp"
#include "tarantino/crawler/adapters/datalake/time/TimeBasedDatalakeAdapter.hpp"
#include "tarantino/crawler/adapters/gutenberg/GutenbergHttpDownloader.hpp"
#include "tarantino/crawler/adapters/gutenberg/LocalMirrorDownloader.hpp"
#include "tarantino/platform/Paths.hpp"

#include <stdexcept>

namespace tarantino::crawler {

commands::IngestBookCommand CrawlerFactory::ingestCommand(const CrawlerConfig& config) {
    return commands::IngestBookCommand(downloader(config), datalake(config));
}

int CrawlerFactory::removeIncompleteWrites(const CrawlerConfig& config, std::ostream& out) {
    const int removed = datalake(config)->removeIncompleteWrites();
    if (removed > 0) {
        out << "Removed " << removed << " files left by an interrupted run from " << platform::portable(config.datalake)
            << std::endl;
    }
    return removed;
}

std::shared_ptr<ports::BookDownloader> CrawlerFactory::downloader(const CrawlerConfig& config) {
    if (config.mirror) {
        return std::make_shared<adapters::LocalMirrorDownloader>(*config.mirror);
    }
    return std::make_shared<adapters::GutenbergHttpDownloader>();
}

std::vector<int> CrawlerFactory::mirrorBookIds(const CrawlerConfig& config) {
    return adapters::LocalMirrorDownloader(config.mirror.value_or(std::filesystem::path{})).bookIds();
}

std::shared_ptr<ports::DatalakeStorage> CrawlerFactory::datalake(const CrawlerConfig& config) {
    if (config.datalakeLayout == "time") {
        return std::make_shared<adapters::TimeBasedDatalakeAdapter>(config.datalake);
    }
    if (config.datalakeLayout == "book") {
        return std::make_shared<adapters::BookBasedDatalakeAdapter>(config.datalake);
    }
    if (config.datalakeLayout == "batch") {
        return std::make_shared<adapters::BatchBasedDatalakeAdapter>(config.datalake);
    }
    throw std::invalid_argument("Unknown datalake layout: " + config.datalakeLayout);
}

}
