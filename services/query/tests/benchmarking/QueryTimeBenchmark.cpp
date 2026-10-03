#include "query/tests/benchmarking/BenchmarkRunner.hpp"

#include "crawler/tests/benchmarking/support/environment/Heap.hpp"
#include "crawler/tests/benchmarking/support/files/FootprintLog.hpp"
#include "crawler/tests/benchmarking/support/validation/Check.hpp"
#include "indexer/tests/benchmarking/support/BenchmarkStore.hpp"
#include "indexer/tests/benchmarking/support/IndexFixture.hpp"
#include "indexer/tests/benchmarking/support/PrebuiltIndexes.hpp"
#include "query/tests/benchmarking/QueryWorkload.hpp"
#include "query/tests/benchmarking/ReferenceResults.hpp"
#include "tarantino/platform/Files.hpp"
#include "tarantino/query/model/QueryTerms.hpp"

#include <algorithm>
#include <chrono>
#include <map>
#include <optional>
#include <random>

namespace tarantino::query::benchmarking {

using crawler::benchmarking::BenchmarkState;
using crawler::benchmarking::Blackhole;
using crawler::benchmarking::Check;
using crawler::benchmarking::FootprintLog;
using crawler::benchmarking::Heap;
using crawler::benchmarking::ResultRow;
using indexer::benchmarking::BenchmarkStore;
using indexer::benchmarking::IndexFixture;
using indexer::benchmarking::PrebuiltIndexes;

namespace {

// The first opening of each process is discarded: for mongo it also creates the client
constexpr int INDEX_MEMORY_SAMPLES = 5;

using Clock = std::chrono::steady_clock;

class QueryTimeState : public BenchmarkState {
public:
    QueryTimeState(const std::string& index, int books) : QueryTimeState(index, books, IndexFixture::fromEnvironment()) {
    }

    void setupIteration(bool) override {
        nanosPerCategory_.assign(categories_.size(), 0);
        queriesPerCategory_.assign(categories_.size(), 0);
    }

    /// A random query of any category, timed per category too, so one run measures every category (SPEC §11). The
    /// result is freed inside both times, as the harness times the whole call.
    void invoke(const std::string&, Blackhole& blackhole) override {
        const std::size_t query = pick_(random_);
        const Clock::time_point start = Clock::now();
        const std::size_t found = search_->execute(queries_[query]).books.size();
        nanosPerCategory_[categoryOfQuery_[query]] += static_cast<std::uint64_t>(std::chrono::nanoseconds(Clock::now() - start).count());
        ++queriesPerCategory_[categoryOfQuery_[query]];
        blackhole.consume(static_cast<std::uint64_t>(found));
    }

    void tearDownIteration(bool measured) override {
        if (!measured) {
            return;
        }
        for (std::size_t category = 0; category < categories_.size(); ++category) {
            if (queriesPerCategory_[category] == 0) {
                continue;
            }
            const double microsPerQuery = static_cast<double>(nanosPerCategory_[category]) / 1000.0 / static_cast<double>(queriesPerCategory_[category]);
            FootprintLog::appendSample(QUERY_SERVICE, ResultRow::sample(index_, "query_time_" + categories_[category], books_, microsPerQuery, QUERY_TIME_UNIT));
        }
    }

private:
    QueryTimeState(const std::string& index, int books, IndexFixture fixture)
        : index_(index),
          books_(books),
          store_(PrebuiltIndexes::of(index, books, fixture)),
          stopwords_(QueryWorkload::stopwords()),
          random_(std::random_device{}()) {
        const std::vector<WorkloadQuery> workload = QueryWorkload::queries();
        Check::require(!workload.empty(), "the workload has no queries");
        queries_ = QueryWorkload::texts(workload, QueryWorkload::ALL_CATEGORIES);
        for (const WorkloadQuery& query : workload) {
            const auto known = std::find(categories_.begin(), categories_.end(), query.category);
            categoryOfQuery_.push_back(static_cast<std::size_t>(known - categories_.begin()));
            if (known == categories_.end()) {
                categories_.push_back(query.category);
            }
        }
        pick_ = std::uniform_int_distribution<std::size_t>(0, queries_.size() - 1);
        search_.emplace(openAndAnswerEveryQuery());
        for (int sample = 0; sample < INDEX_MEMORY_SAMPLES; ++sample) {
            recordIndexMemory();
        }
        requireReferenceResults(fixture);
    }

    /// The memory in use once the index is open and has answered every query, minus the same before (SPEC §11).
    void recordIndexMemory() {
        search_.reset();
        const auto before = static_cast<double>(Heap::inUse());
        search_.emplace(openAndAnswerEveryQuery());
        const double indexMemory = static_cast<double>(Heap::inUse()) - before;
        FootprintLog::appendSample(QUERY_SERVICE, ResultRow::sample(index_, "index_memory", books_, indexMemory, "bytes"));
    }

    commands::SearchCommand openAndAnswerEveryQuery() {
        commands::SearchCommand opened = QueryWorkload::openSearch(store_, index_, stopwords_);
        for (const std::string& query : queries_) {
            opened.execute(query);
        }
        return opened;
    }

    void requireReferenceResults(IndexFixture& fixture) {
        const std::map<std::string, std::vector<int>> expected = referenceResults(fixture);
        for (const std::string& query : queries_) {
            const auto reference = expected.find(query);
            Check::require(reference != expected.end() && resultIds(query) == reference->second, "wrong result for query '" + query + "'");
        }
    }

    std::map<std::string, std::vector<int>> referenceResults(IndexFixture& fixture) {
        const std::filesystem::path cached = PrebuiltIndexes::file("reference-results-" + std::to_string(books_) + ".tsv");
        if (platform::files::exists(cached)) {
            return ReferenceResults::read(cached);
        }
        std::map<std::string, std::vector<int>> expected = computedReferenceResults(fixture);
        ReferenceResults::write(cached, expected);
        return expected;
    }

    /// The books that hold every term of each query, from the terms of the books themselves instead of an index.
    std::map<std::string, std::vector<int>> computedReferenceResults(IndexFixture& fixture) {
        std::map<std::string, std::vector<std::string>> termsOfQuery;
        std::map<std::string, std::vector<int>> expected;
        for (const std::string& query : queries_) {
            termsOfQuery.emplace(query, model::QueryTerms::of(query, stopwords_));
            expected.emplace(query, std::vector<int>{});
        }
        std::vector<int> ids = fixture.dataset().ids(books_);
        std::sort(ids.begin(), ids.end());
        for (const int bookId : ids) {
            const platform::StringSet bookTerms = fixture.terms(bookId);
            for (const auto& [query, terms] : termsOfQuery) {
                const bool containsAll = std::all_of(terms.begin(), terms.end(), [&](const std::string& term) { return bookTerms.contains(term); });
                if (!terms.empty() && containsAll) {
                    expected[query].push_back(bookId);
                }
            }
        }
        return expected;
    }

    std::vector<int> resultIds(const std::string& query) {
        const model::SearchResult result = search_->execute(query);
        std::vector<int> ids;
        ids.reserve(result.books.size());
        for (const model::BookMetadata& book : result.books) {
            ids.push_back(book.bookId);
        }
        return ids;
    }

    std::string index_;
    int books_;
    BenchmarkStore store_;
    platform::StringSet stopwords_;
    std::vector<std::string> queries_;
    std::vector<std::string> categories_;
    std::vector<std::size_t> categoryOfQuery_;
    std::optional<commands::SearchCommand> search_;
    std::vector<std::uint64_t> nanosPerCategory_;
    std::vector<std::uint64_t> queriesPerCategory_;
    std::mt19937_64 random_;
    std::uniform_int_distribution<std::size_t> pick_;
};

}

Benchmark QueryTimeBenchmark::definition() {
    return Benchmark{"QueryTimeBenchmark", {"queryTime"}, crawler::benchmarking::Mode::SampleTime,
                     crawler::benchmarking::TimeUnit::Microseconds, crawler::benchmarking::Iterations{3, 5, std::chrono::seconds(1)},
                     "index", indexStructures(), {100, 300, 1000},
                     [](const std::string& index, int books) { return std::make_unique<QueryTimeState>(index, books); }};
}

}
