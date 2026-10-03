#include "query/tests/benchmarking/BenchmarkRunner.hpp"

#include "crawler/tests/benchmarking/support/environment/BenchmarkPaths.hpp"
#include "crawler/tests/benchmarking/support/validation/Check.hpp"
#include "indexer/tests/benchmarking/support/BenchmarkStore.hpp"
#include "indexer/tests/benchmarking/support/IndexFixture.hpp"
#include "tarantino/query/QueryConfig.hpp"
#include "tarantino/query/QueryFactory.hpp"

#include <algorithm>
#include <random>

namespace tarantino::query::benchmarking {

using crawler::benchmarking::BenchmarkPaths;
using crawler::benchmarking::BenchmarkState;
using crawler::benchmarking::Blackhole;
using crawler::benchmarking::Check;
using indexer::benchmarking::BenchmarkStore;
using indexer::benchmarking::IndexFixture;

namespace {

class MetadataQueryState : public BenchmarkState {
public:
    MetadataQueryState(const std::string& metadata, int books)
        : metadata_(metadata),
          store_(BenchmarkStore::forMetadata(metadata, "query-metadata-" + metadata + "-" + std::to_string(books))),
          random_(std::random_device{}()) {
        store_.clear();
        IndexFixture fixture = IndexFixture::fromEnvironment();
        ids_ = fixture.dataset().ids(books);
        const std::vector<indexer::model::Book> parsedBooks = fixture.books(ids_);
        fixture.save(*store_.metadata(), parsedBooks);
        for (const indexer::model::Book& book : parsedBooks) {
            if (book.author && std::find(authors_.begin(), authors_.end(), *book.author) == authors_.end()) {
                authors_.push_back(*book.author);
            }
        }
        reader_ = QueryFactory::metadata(config());
        for (const int bookId : ids_) {
            requireBook(bookId);
        }
        for (const indexer::model::Book& book : parsedBooks) {
            if (book.author) {
                requireAmongBooksOfItsAuthor(book);
            }
        }
        Check::require(!authors_.empty(), "the books have no author");
        pickBook_ = std::uniform_int_distribution<std::size_t>(0, ids_.size() - 1);
        pickAuthor_ = std::uniform_int_distribution<std::size_t>(0, authors_.size() - 1);
    }

    void invoke(const std::string& method, Blackhole& blackhole) override {
        if (method == "bookByIdTime") {
            blackhole.consume(reader_->book(ids_[pickBook_(random_)]).has_value());
        } else {
            blackhole.consume(static_cast<std::uint64_t>(reader_->booksBy(authors_[pickAuthor_(random_)]).size()));
        }
    }

    /// The reader is closed first: an open database file cannot be deleted on Windows.
    void tearDownTrial() override {
        reader_.reset();
        store_.clear();
    }

private:
    void requireBook(int bookId) {
        const std::optional<model::BookMetadata> book = reader_->book(bookId);
        Check::require(book && book->bookId == bookId, "book " + std::to_string(bookId) + " not found");
    }

    void requireAmongBooksOfItsAuthor(const indexer::model::Book& book) {
        const std::vector<model::BookMetadata> candidates = reader_->booksBy(*book.author);
        const bool found = std::any_of(candidates.begin(), candidates.end(),
                                       [&](const model::BookMetadata& candidate) { return candidate.bookId == book.bookId; });
        Check::require(found, "book " + std::to_string(book.bookId) + " missing from the books of its author");
    }

    QueryConfig config() const {
        return QueryConfig{store_.datamarts(), "json", metadata_, store_.mongoUri(), BenchmarkPaths::workload()};
    }

    std::string metadata_;
    BenchmarkStore store_;
    std::vector<int> ids_;
    std::vector<std::string> authors_;
    std::shared_ptr<ports::MetadataReader> reader_;
    std::mt19937_64 random_;
    std::uniform_int_distribution<std::size_t> pickBook_;
    std::uniform_int_distribution<std::size_t> pickAuthor_;
};

}

Benchmark MetadataQueryBenchmark::definition() {
    return Benchmark{"MetadataQueryBenchmark", {"bookByIdTime", "booksByAuthorTime"}, crawler::benchmarking::Mode::AverageTime,
                     crawler::benchmarking::TimeUnit::Microseconds, crawler::benchmarking::Iterations{3, 5, std::chrono::seconds(1)},
                     "metadata", metadataBackends(), {100, 300, 1000},
                     [](const std::string& metadata, int books) { return std::make_unique<MetadataQueryState>(metadata, books); }};
}

}
