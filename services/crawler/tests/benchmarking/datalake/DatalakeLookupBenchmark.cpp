#include "crawler/tests/benchmarking/datalake/DatalakeBenchmarks.hpp"

#include "crawler/tests/benchmarking/support/datalake/DatalakeFixture.hpp"
#include "crawler/tests/benchmarking/support/environment/BenchmarkPaths.hpp"
#include "crawler/tests/benchmarking/support/files/Directories.hpp"
#include "crawler/tests/benchmarking/support/validation/Check.hpp"
#include "tarantino/platform/Files.hpp"

#include <random>

namespace tarantino::crawler::benchmarking {

namespace {

class DatalakeLookupState : public BenchmarkState {
public:
    DatalakeLookupState(const std::string& layout, int books)
        : root_(BenchmarkPaths::scratch("datalake-lookup-" + layout + "-" + std::to_string(books))),
          random_(std::random_device{}()) {
        BenchmarkDataset dataset = BenchmarkDataset::fromEnvironment();
        ids_ = dataset.ids(books);
        pick_ = std::uniform_int_distribution<std::size_t>(0, ids_.size() - 1);
        Directories::remove(root_);
        DatalakeFixture::ingestAsCrawled(layout, root_, ids_, dataset, DatalakeFixture::CRAWL_START);
        datalake_ = DatalakeFixture::datalake(layout, root_);
        for (const int bookId : ids_) {
            const auto paths = datalake_->pathsOf(bookId);
            Check::require(paths && platform::files::exists(paths->header) && platform::files::exists(paths->body),
                           "book " + std::to_string(bookId) + " not found");
        }
    }

    /// A random book of the dataset, drawn inside the timed call (SPEC §11).
    void invoke(const std::string&, Blackhole& blackhole) override {
        blackhole.consume(datalake_->pathsOf(ids_[pick_(random_)]).has_value());
    }

    void tearDownTrial() override {
        datalake_.reset();
        Directories::remove(root_);
    }

private:
    std::filesystem::path root_;
    std::vector<int> ids_;
    std::shared_ptr<ports::DatalakeStorage> datalake_;
    std::mt19937_64 random_;
    std::uniform_int_distribution<std::size_t> pick_;
};

}

Benchmark DatalakeLookupBenchmark::definition() {
    return Benchmark{"DatalakeLookupBenchmark", {"lookupTime"}, Mode::AverageTime, TimeUnit::Microseconds,
                     Iterations{3, 5, std::chrono::seconds(1)}, "layout", DatalakeFixture::LAYOUTS, {100, 300, 1000},
                     [](const std::string& layout, int books) { return std::make_unique<DatalakeLookupState>(layout, books); }};
}

}
