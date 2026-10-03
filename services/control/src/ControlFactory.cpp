#include "tarantino/control/ControlFactory.hpp"

#include "tarantino/control/adapters/FileControlStateStore.hpp"
#include "tarantino/control/adapters/LocalCrawler.hpp"
#include "tarantino/control/adapters/LocalIndexer.hpp"
#include "tarantino/crawler/CrawlerFactory.hpp"
#include "tarantino/indexer/IndexerFactory.hpp"
#include "tarantino/platform/Files.hpp"
#include "tarantino/platform/Paths.hpp"
#include "tarantino/platform/Text.hpp"
#include "tarantino/platform/ThreadPool.hpp"

#include <stdexcept>

namespace tarantino::control {

namespace {

constexpr const char* DEFAULT_CANDIDATES = "sample_ids.txt";

std::vector<int> ids(const std::filesystem::path& file) {
    std::vector<int> ids;
    for (const std::string& line : platform::files::readLines(file)) {
        const std::string_view text = platform::strip(line);
        if (text.empty()) {
            continue;
        }
        const auto bookId = platform::parseDigits(text);
        if (!bookId || *bookId <= 0) {
            throw std::invalid_argument("Not a book id in " + platform::portable(file) + ": " + std::string(text));
        }
        ids.push_back(*bookId);
    }
    return ids;
}

}

std::unique_ptr<commands::ControlPipeline> ControlFactory::pipeline(const ControlConfig& config,
                                                                    const crawler::CrawlerConfig& crawler,
                                                                    const indexer::IndexerConfig& indexer,
                                                                    std::vector<int> candidates, std::ostream& out) {
    crawler::CrawlerFactory::removeIncompleteWrites(crawler, out);
    // The pool lives as long as the crawler that downloads with it (SPEC §14: a pool of D threads)
    auto downloads = std::make_shared<platform::ThreadPool>(config.parallelDownloads);
    auto localCrawler = std::make_shared<adapters::LocalCrawler>(
            crawler::CrawlerFactory::ingestCommand(crawler),
            [downloads](std::function<void()> download) { downloads->execute(std::move(download)); });
    auto localIndexer = std::make_shared<adapters::LocalIndexer>(indexer::IndexerFactory::indexCommand(indexer));
    return std::make_unique<commands::ControlPipeline>(std::make_shared<adapters::FileControlStateStore>(config.control),
                                                       std::move(localCrawler), std::move(localIndexer),
                                                       std::move(candidates), config.parallelDownloads, config.indexBatch);
}

std::vector<int> ControlFactory::candidates(const ControlConfig& config, const crawler::CrawlerConfig& crawler,
                                            const std::optional<std::string>& file) {
    if (!file && crawler.mirror) {
        return crawler::CrawlerFactory::mirrorBookIds(crawler);
    }
    return ids(config.workload / platform::pathOf(file.value_or(DEFAULT_CANDIDATES)));
}

}
