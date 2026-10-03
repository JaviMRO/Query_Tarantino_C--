#include "indexer/tests/benchmarking/BenchmarkRunner.hpp"

#include "indexer/tests/benchmarking/support/BenchmarkStore.hpp"
#include "indexer/tests/benchmarking/support/IndexFixture.hpp"

namespace tarantino::indexer::benchmarking {

using crawler::benchmarking::BenchmarkState;
using crawler::benchmarking::Blackhole;

namespace {

class MetadataInsertionState : public BenchmarkState {
public:
    MetadataInsertionState(const std::string& metadata, int books)
        : fixture_(IndexFixture::fromEnvironment()),
          store_(BenchmarkStore::forMetadata(metadata, "metadata-insert-" + metadata + "-" + std::to_string(books))),
          parsedBooks_(fixture_.books(fixture_.dataset().ids(books))) {
    }

    void setupIteration(bool) override {
        store_.clear();
    }

    /// The headers are parsed before timing; the run creates the backend, opens it and saves the N books (SPEC §11).
    void invoke(const std::string&, Blackhole& blackhole) override {
        metadata_ = store_.metadata();
        fixture_.save(*metadata_, parsedBooks_);
        blackhole.consume(static_cast<std::uint64_t>(parsedBooks_.size()));
    }

    /// Closed outside the timed run, before the next one empties the backend
    void tearDownIteration(bool) override {
        metadata_.reset();
    }

    void tearDownTrial() override {
        store_.clear();
    }

private:
    IndexFixture fixture_;
    BenchmarkStore store_;
    std::vector<model::Book> parsedBooks_;
    std::shared_ptr<ports::MetadataStorage> metadata_;
};

}

Benchmark MetadataInsertionBenchmark::definition() {
    return Benchmark{"MetadataInsertionBenchmark", {"bulkInsertionTime"}, crawler::benchmarking::Mode::SingleShotTime,
                     crawler::benchmarking::TimeUnit::Milliseconds, crawler::benchmarking::Iterations{1, 3, std::chrono::seconds(1)},
                     "metadata", metadataBackends(), {100, 300, 1000},
                     [](const std::string& metadata, int books) { return std::make_unique<MetadataInsertionState>(metadata, books); }};
}

}
