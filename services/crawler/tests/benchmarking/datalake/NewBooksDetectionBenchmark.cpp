#include "crawler/tests/benchmarking/datalake/DatalakeBenchmarks.hpp"

#include "crawler/tests/benchmarking/support/datalake/CrawlClock.hpp"
#include "crawler/tests/benchmarking/support/datalake/DatalakeFixture.hpp"
#include "crawler/tests/benchmarking/support/environment/BenchmarkPaths.hpp"
#include "crawler/tests/benchmarking/support/files/Directories.hpp"
#include "crawler/tests/benchmarking/support/validation/Check.hpp"
#include "tarantino/platform/Files.hpp"

namespace tarantino::crawler::benchmarking {

namespace {

constexpr std::chrono::hours AGE_OF_OLD_BOOKS{24};

class NewBooksDetectionState : public BenchmarkState {
public:
    NewBooksDetectionState(std::string layout, int books)
        : layout_(std::move(layout)),
          root_(BenchmarkPaths::scratch("datalake-detection-" + layout_ + "-" + std::to_string(books))),
          lastRun_(platform::now()) {
        BenchmarkDataset dataset = BenchmarkDataset::fromEnvironment();
        Directories::remove(root_);
        lastRun_ = platform::now();
        storeOldBooks(dataset.ids(books), dataset);
        datalake_ = DatalakeFixture::datalake(layout_, root_);
        const std::vector<int> newIds = dataset.newIds();
        DatalakeFixture::ingest(datalake_, newIds, dataset);
        Check::require(datalake_->idsStoredSince(lastRun_) == std::unordered_set<int>(newIds.begin(), newIds.end()),
                       "detection did not list exactly the new books");
    }

    void invoke(const std::string&, Blackhole& blackhole) override {
        blackhole.consume(datalake_->idsStoredSince(lastRun_).size());
    }

    void tearDownTrial() override {
        datalake_.reset();
        Directories::remove(root_);
    }

private:
    /// The dataset as a crawl that ended one day before the new books saved it, with the modification time of each
    /// body set to its save (SPEC §11).
    void storeOldBooks(const std::vector<int>& ids, BenchmarkDataset& dataset) {
        const platform::Instant crawlStart = lastRun_ - AGE_OF_OLD_BOOKS - CrawlClock::durationOf(static_cast<int>(ids.size()));
        const std::vector<model::StoredPaths> stored = DatalakeFixture::ingestAsCrawled(layout_, root_, ids, dataset, crawlStart);
        for (std::size_t position = 0; position < stored.size(); ++position) {
            platform::files::setLastModified(stored[position].body, CrawlClock::instantOf(crawlStart, static_cast<int>(position)));
        }
    }

    std::string layout_;
    std::filesystem::path root_;
    platform::Instant lastRun_;
    std::shared_ptr<ports::DatalakeStorage> datalake_;
};

}

Benchmark NewBooksDetectionBenchmark::definition() {
    return Benchmark{"NewBooksDetectionBenchmark", {"newBooksDetectionTime"}, Mode::AverageTime, TimeUnit::Milliseconds,
                     Iterations{3, 5, std::chrono::seconds(1)}, "layout", DatalakeFixture::LAYOUTS, {100, 300, 1000},
                     [](const std::string& layout, int books) { return std::make_unique<NewBooksDetectionState>(layout, books); }};
}

}
