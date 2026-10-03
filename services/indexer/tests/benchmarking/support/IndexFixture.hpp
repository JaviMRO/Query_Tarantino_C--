#pragma once

#include "crawler/tests/benchmarking/support/dataset/BenchmarkDataset.hpp"
#include "tarantino/indexer/model/book/Book.hpp"
#include "tarantino/indexer/model/book/HeaderParser.hpp"
#include "tarantino/indexer/model/terms/Tokenizer.hpp"
#include "tarantino/indexer/ports/datamarts/InvertedIndexStorage.hpp"
#include "tarantino/indexer/ports/datamarts/MetadataStorage.hpp"
#include "tarantino/platform/Text.hpp"

#include <cstdint>
#include <vector>

namespace tarantino::indexer::benchmarking {

/// The books of the dataset as the indexer indexes them: read from the cache, split, tokenized and added (SPEC §11).
class IndexFixture {
public:
    IndexFixture(crawler::benchmarking::BenchmarkDataset dataset, model::Tokenizer tokenizer);

    static IndexFixture fromEnvironment();

    crawler::benchmarking::BenchmarkDataset& dataset();

    /// Adds the books and flushes the index once.
    void index(ports::InvertedIndexStorage& invertedIndex, const std::vector<int>& ids);

    void add(ports::InvertedIndexStorage& invertedIndex, const std::vector<int>& ids);

    void save(ports::MetadataStorage& metadata, const std::vector<model::Book>& books);

    std::vector<model::Book> books(const std::vector<int>& ids);

    /// Distinct terms of the books, which every index structure must hold once they are indexed.
    std::uint64_t vocabularySize(const std::vector<int>& ids);

    platform::StringSet terms(int bookId);

private:
    model::TermOccurrences occurrences(int bookId);

    model::BookText bookText(int bookId);

    crawler::benchmarking::BenchmarkDataset dataset_;
    model::Tokenizer tokenizer_;
    model::HeaderParser headerParser_;
};

}
