#include "query/tests/benchmarking/BenchmarkRunner.hpp"

#include "crawler/tests/benchmarking/support/validation/Check.hpp"
#include "indexer/tests/benchmarking/support/BenchmarkStore.hpp"
#include "indexer/tests/benchmarking/support/IndexFixture.hpp"
#include "indexer/tests/benchmarking/support/PrebuiltIndexes.hpp"
#include "query/tests/benchmarking/QueryWorkload.hpp"

#include <optional>

namespace tarantino::query::benchmarking {

using crawler::benchmarking::BenchmarkState;
using crawler::benchmarking::Blackhole;
using crawler::benchmarking::Check;
using indexer::benchmarking::BenchmarkStore;
using indexer::benchmarking::IndexFixture;
using indexer::benchmarking::PrebuiltIndexes;

namespace {

BenchmarkStore prebuiltIndex(const std::string& index, int books) {
    IndexFixture fixture = IndexFixture::fromEnvironment();
    return PrebuiltIndexes::of(index, books, fixture);
}

std::string firstQuery() {
    const std::vector<WorkloadQuery> queries = QueryWorkload::queries();
    Check::require(!queries.empty(), "the workload has no queries");
    return queries.front().text;
}

class IndexOpenState : public BenchmarkState {
public:
    IndexOpenState(const std::string& index, int books)
        : index_(index), store_(prebuiltIndex(index, books)), stopwords_(QueryWorkload::stopwords()), firstQuery_(firstQuery()) {
    }

    void invoke(const std::string&, Blackhole& blackhole) override {
        opened_.emplace(QueryWorkload::openSearch(store_, index_, stopwords_));
        blackhole.consume(static_cast<std::uint64_t>(opened_->execute(firstQuery_).books.size()));
    }

    /// Released after the timed run, which measures opening the index and answering a query, not freeing it.
    void tearDownIteration(bool) override {
        opened_.reset();
    }

private:
    std::string index_;
    BenchmarkStore store_;
    platform::StringSet stopwords_;
    std::string firstQuery_;
    std::optional<commands::SearchCommand> opened_;
};

}

Benchmark IndexOpenBenchmark::definition() {
    return Benchmark{"IndexOpenBenchmark", {"indexOpenTime"}, crawler::benchmarking::Mode::SingleShotTime,
                     crawler::benchmarking::TimeUnit::Milliseconds, crawler::benchmarking::Iterations{1, 3, std::chrono::seconds(1)},
                     "index", indexStructures(), {100, 300, 1000},
                     [](const std::string& index, int books) { return std::make_unique<IndexOpenState>(index, books); }};
}

}
