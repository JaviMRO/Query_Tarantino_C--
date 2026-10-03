#include "indexer/tests/benchmarking/BenchmarkRunner.hpp"

#include "crawler/tests/benchmarking/support/environment/Heap.hpp"
#include "crawler/tests/benchmarking/support/files/FootprintLog.hpp"
#include "crawler/tests/benchmarking/support/validation/Check.hpp"
#include "indexer/tests/benchmarking/support/BenchmarkStore.hpp"
#include "indexer/tests/benchmarking/support/IndexFixture.hpp"
#include "indexer/tests/benchmarking/support/PrebuiltIndexes.hpp"
#include "tarantino/platform/Files.hpp"
#include "tarantino/platform/Text.hpp"

namespace tarantino::indexer::benchmarking {

using crawler::benchmarking::BenchmarkState;
using crawler::benchmarking::Blackhole;
using crawler::benchmarking::Check;
using crawler::benchmarking::FootprintLog;
using crawler::benchmarking::Heap;
using crawler::benchmarking::ResultRow;

namespace {

constexpr int WARM_UP_BOOKS = 100;
// Adding without flushing is cheap next to a build, and 2 processes give 6 samples instead of 2
constexpr int BUILD_MEMORY_SAMPLES = 3;

class FullIndexBuildState : public BenchmarkState {
public:
    FullIndexBuildState(std::string index, int books)
        : index_(std::move(index)),
          books_(books),
          fixture_(IndexFixture::fromEnvironment()),
          store_(BenchmarkStore::forIndex(index_, "index-build-" + index_ + "-" + std::to_string(books))),
          ids_(fixture_.dataset().ids(books)),
          referenceTermCount_(referenceTermCount()) {
        warmUp();
    }

    void setupIteration(bool) override {
        store_.clear();
    }

    /// The N books read, split, tokenized and indexed into an empty index, flushed once at the end (SPEC §11).
    void invoke(const std::string&, Blackhole& blackhole) override {
        const std::uint64_t before = Heap::allocatedSoFar();
        built_ = store_.invertedIndex();
        fixture_.index(*built_, ids_);
        allocated_ = Heap::allocatedSoFar() - before;
        blackhole.consume(allocated_);
    }

    // Measured here, around the build alone, never around emptying the storage
    void tearDownIteration(bool measured) override {
        built_.reset();
        if (measured) {
            FootprintLog::appendSample(INDEXER_SERVICE, ResultRow::sample(index_, "memory_allocated", books_, static_cast<double>(allocated_), "bytes"));
        }
    }

    void tearDownTrial() override {
        const StoreFootprint footprint = store_.footprint();
        Check::require(footprint.terms == referenceTermCount_, index_ + " holds " + std::to_string(footprint.terms)
                                                                     + " terms instead of the " + std::to_string(referenceTermCount_) + " of the books");
        FootprintLog::append(INDEXER_SERVICE, {ResultRow::exact(index_, "disk_usage", books_, static_cast<double>(footprint.bytes), "bytes"),
                                               ResultRow::exact(index_, "disk_allocated", books_, static_cast<double>(footprint.allocatedBytes), "bytes"),
                                               ResultRow::exact(index_, "term_count", books_, static_cast<double>(footprint.terms), "terms")});
        for (int sample = 0; sample < BUILD_MEMORY_SAMPLES; ++sample) {
            store_.clear();
            FootprintLog::appendSample(INDEXER_SERVICE, ResultRow::sample(index_, "build_memory", books_, buildMemory(), "bytes"));
        }
        store_.clear();
    }

private:
    /// Warms up with the same code on 100 books instead of a whole warm-up build of N (SPEC §11), and reads the N texts
    /// once so that the first measured build finds them in the operating system cache, like the others.
    void warmUp() {
        for (const int bookId : ids_) {
            fixture_.dataset().rawText(bookId);
        }
        const BenchmarkStore warmUp = BenchmarkStore::forIndex(index_, "index-build-warmup-" + index_);
        warmUp.clear();
        fixture_.index(*warmUp.invertedIndex(), fixture_.dataset().ids(WARM_UP_BOOKS));
        warmUp.clear();
    }

    std::uint64_t referenceTermCount() {
        const std::filesystem::path cached = PrebuiltIndexes::file("term-count-" + std::to_string(books_) + ".txt");
        if (platform::files::exists(cached)) {
            return static_cast<std::uint64_t>(std::stoull(std::string(platform::strip(platform::files::readString(cached)))));
        }
        const std::uint64_t termCount = fixture_.vocabularySize(ids_);
        platform::files::createDirectories(cached.parent_path());
        platform::files::writeString(cached, std::to_string(termCount));
        return termCount;
    }

    /// The memory in use once the N books are added and before the flush, minus the same before (SPEC §11).
    double buildMemory() {
        const auto before = static_cast<double>(Heap::inUse());
        const auto invertedIndex = store_.invertedIndex();
        fixture_.add(*invertedIndex, ids_);
        return static_cast<double>(Heap::inUse()) - before;
    }

    std::string index_;
    int books_;
    IndexFixture fixture_;
    BenchmarkStore store_;
    std::vector<int> ids_;
    std::uint64_t referenceTermCount_;
    std::shared_ptr<ports::InvertedIndexStorage> built_;
    std::uint64_t allocated_ = 0;
};

}

Benchmark FullIndexBuildBenchmark::definition() {
    return Benchmark{"FullIndexBuildBenchmark", {"fullBuildTime"}, crawler::benchmarking::Mode::SingleShotTime,
                     crawler::benchmarking::TimeUnit::Milliseconds, crawler::benchmarking::Iterations{0, 3, std::chrono::seconds(1)},
                     "index", indexStructures(), {100, 300, 1000},
                     [](const std::string& index, int books) { return std::make_unique<FullIndexBuildState>(index, books); }};
}

}
