#include "indexer/tests/benchmarking/BenchmarkRunner.hpp"

#include "indexer/tests/benchmarking/support/BenchmarkStore.hpp"
#include "indexer/tests/benchmarking/support/IndexFixture.hpp"
#include "indexer/tests/benchmarking/support/PrebuiltIndexes.hpp"

namespace tarantino::indexer::benchmarking {

using crawler::benchmarking::BenchmarkState;
using crawler::benchmarking::Blackhole;

namespace {

class IncrementalUpdateState : public BenchmarkState {
public:
    IncrementalUpdateState(const std::string& index, int books)
        : index_(index),
          fixture_(IndexFixture::fromEnvironment()),
          snapshot_(PrebuiltIndexes::of(index, books, fixture_)),
          store_(PrebuiltIndexes::workingCopyOf(snapshot_, index, books)),
          newIds_(fixture_.dataset().newIds()),
          booksFlushedOneByOne_(newIds_.begin(), newIds_.begin() + IncrementalUpdateBenchmark::BOOKS_FLUSHED_ONE_BY_ONE) {
        for (const int bookId : newIds_) {
            for (const std::string& term : fixture_.terms(bookId)) {
                touchedTerms_.insert(term);
            }
        }
        warmUp();
    }

    /// Puts back the terms of the new books right before every run, so each one starts from the prebuilt index just
    /// written (SPEC §11), and opens the index before timing, as a running control layer has it open: otherwise json's
    /// loading would be spread over 10 books in one method and over 100 in the other. index_open_time measures it.
    void setupIteration(bool) override {
        store_.restoreTermsFrom(snapshot_, touchedTerms_);
        invertedIndex_ = store_.invertedIndex();
        invertedIndex_->open();
    }

    void invoke(const std::string& method, Blackhole& blackhole) override {
        if (method == "incrementalUpdateTime") {
            // As the control layer indexes with K = 1 (SPEC §9): an open index, flushed after every book
            for (const int bookId : booksFlushedOneByOne_) {
                fixture_.index(*invertedIndex_, {bookId});
            }
        } else {
            // As the control layer indexes with the default K = 100: an open index, flushed once
            fixture_.index(*invertedIndex_, newIds_);
        }
        blackhole.consume(static_cast<std::uint64_t>(newIds_.size()));
    }

    void tearDownIteration(bool) override {
        invertedIndex_.reset();
    }

private:
    /// Warms up with the same code instead of a whole warm-up run (SPEC §11): the first new books indexed into an empty
    /// index one by one, which creates their term files, and then together, which merges them.
    void warmUp() {
        const BenchmarkStore warmUp = BenchmarkStore::forIndex(index_, "index-update-warmup-" + index_);
        warmUp.clear();
        {
            const auto invertedIndex = warmUp.invertedIndex();
            for (const int bookId : booksFlushedOneByOne_) {
                fixture_.index(*invertedIndex, {bookId});
            }
        }
        fixture_.index(*warmUp.invertedIndex(), booksFlushedOneByOne_);
        warmUp.clear();
    }

    std::string index_;
    IndexFixture fixture_;
    BenchmarkStore snapshot_;
    BenchmarkStore store_;
    std::vector<int> newIds_;
    std::vector<int> booksFlushedOneByOne_;
    std::unordered_set<std::string> touchedTerms_;
    std::shared_ptr<ports::InvertedIndexStorage> invertedIndex_;
};

}

Benchmark IncrementalUpdateBenchmark::definition() {
    return Benchmark{"IncrementalUpdateBenchmark", {"incrementalUpdateTime", "batchUpdateTime"},
                     crawler::benchmarking::Mode::SingleShotTime, crawler::benchmarking::TimeUnit::Milliseconds,
                     crawler::benchmarking::Iterations{0, 3, std::chrono::seconds(1)}, "index", indexStructures(), {100, 300, 1000},
                     [](const std::string& index, int books) { return std::make_unique<IncrementalUpdateState>(index, books); }};
}

}
