#include "crawler/tests/benchmarking/support/datalake/DatalakeFixture.hpp"

#include "crawler/tests/benchmarking/support/datalake/CrawlClock.hpp"
#include "crawler/tests/benchmarking/support/files/Directories.hpp"
#include "crawler/tests/benchmarking/support/validation/Check.hpp"
#include "crawler/tests/ports/FunctionDownloader.hpp"
#include "tarantino/crawler/CrawlerFactory.hpp"
#include "tarantino/crawler/adapters/datalake/time/TimeBasedDatalakeAdapter.hpp"
#include "tarantino/crawler/commands/IngestBookCommand.hpp"

namespace tarantino::crawler::benchmarking {

const std::vector<std::string> DatalakeFixture::LAYOUTS{"time", "book", "batch"};
const platform::Instant DatalakeFixture::CRAWL_START = *platform::parseInstant("2025-09-25T00:00:00Z");

namespace {

model::StoredPaths stored(const commands::IngestResult& result) {
    Check::require(result.succeeded(), "book " + std::to_string(result.bookId) + " not ingested: "
                                               + std::string(result.failure ? model::name(*result.failure) : ""));
    return *result.paths;
}

std::shared_ptr<ports::BookDownloader> cacheOf(BenchmarkDataset& dataset) {
    return ports::FunctionDownloader::of([&dataset](int bookId) { return dataset.rawText(bookId); });
}

}

std::shared_ptr<ports::DatalakeStorage> DatalakeFixture::datalake(const std::string& layout, const std::filesystem::path& root,
                                                                  platform::Clock clock) {
    if (layout == "time") {
        return std::make_shared<adapters::TimeBasedDatalakeAdapter>(root, std::move(clock));
    }
    return CrawlerFactory::datalake(CrawlerConfig{root, layout, std::nullopt});
}

std::vector<model::StoredPaths> DatalakeFixture::ingest(const std::shared_ptr<ports::DatalakeStorage>& datalake,
                                                        const std::vector<int>& ids, BenchmarkDataset& dataset) {
    commands::IngestBookCommand ingest(cacheOf(dataset), datalake);
    std::vector<model::StoredPaths> paths;
    paths.reserve(ids.size());
    for (const int bookId : ids) {
        paths.push_back(stored(ingest.execute(bookId)));
    }
    return paths;
}

std::vector<model::StoredPaths> DatalakeFixture::ingestAsCrawled(const std::string& layout, const std::filesystem::path& root,
                                                                 const std::vector<int>& ids, BenchmarkDataset& dataset,
                                                                 platform::Instant start) {
    CrawlClock clock(start);
    commands::IngestBookCommand ingest(cacheOf(dataset), datalake(layout, root, clock.clock()));
    std::vector<model::StoredPaths> paths;
    paths.reserve(ids.size());
    for (std::size_t position = 0; position < ids.size(); ++position) {
        clock.moveTo(static_cast<int>(position));
        paths.push_back(stored(ingest.execute(ids[position])));
    }
    return paths;
}

std::vector<ResultRow> DatalakeFixture::footprint(const std::string& layout, int books, const std::filesystem::path& root) {
    const Footprint measured = Directories::footprint(root);
    return {ResultRow::exact(layout, "file_count", books, static_cast<double>(measured.files), "files"),
            ResultRow::exact(layout, "directory_count", books, static_cast<double>(measured.directories), "dirs"),
            ResultRow::exact(layout, "disk_usage", books, static_cast<double>(measured.bytes), "bytes"),
            ResultRow::exact(layout, "disk_allocated", books, static_cast<double>(measured.allocatedBytes), "bytes")};
}

}
