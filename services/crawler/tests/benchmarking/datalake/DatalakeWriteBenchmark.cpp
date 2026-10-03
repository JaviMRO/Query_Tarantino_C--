#include "crawler/tests/benchmarking/datalake/DatalakeBenchmarks.hpp"

#include "crawler/tests/benchmarking/BenchmarkRunner.hpp"
#include "crawler/tests/benchmarking/support/datalake/DatalakeFixture.hpp"
#include "crawler/tests/benchmarking/support/environment/BenchmarkPaths.hpp"
#include "crawler/tests/benchmarking/support/files/Directories.hpp"
#include "crawler/tests/benchmarking/support/files/FootprintLog.hpp"

namespace tarantino::crawler::benchmarking {

namespace {

class DatalakeWriteState : public BenchmarkState {
public:
    DatalakeWriteState(std::string layout, int books)
        : layout_(std::move(layout)),
          books_(books),
          dataset_(BenchmarkDataset::fromEnvironment()),
          ids_(dataset_.ids(books)),
          root_(BenchmarkPaths::scratch("datalake-write-" + layout_ + "-" + std::to_string(books))) {
    }

    /// Every run starts from an empty datalake (SPEC §11).
    void setupIteration(bool) override {
        Directories::remove(root_);
    }

    void invoke(const std::string&, Blackhole& blackhole) override {
        blackhole.consume(DatalakeFixture::ingestAsCrawled(layout_, root_, ids_, dataset_, DatalakeFixture::CRAWL_START).size());
    }

    void tearDownTrial() override {
        FootprintLog::append(CRAWLER_SERVICE, DatalakeFixture::footprint(layout_, books_, root_));
        Directories::remove(root_);
    }

private:
    std::string layout_;
    int books_;
    BenchmarkDataset dataset_;
    std::vector<int> ids_;
    std::filesystem::path root_;
};

}

Benchmark DatalakeWriteBenchmark::definition() {
    return Benchmark{"DatalakeWriteBenchmark", {"writeThroughput"}, Mode::SingleShotTime, TimeUnit::Milliseconds,
                     Iterations{1, 3, std::chrono::seconds(1)}, "layout", DatalakeFixture::LAYOUTS, {100, 300, 1000},
                     [](const std::string& layout, int books) { return std::make_unique<DatalakeWriteState>(layout, books); }};
}

}
