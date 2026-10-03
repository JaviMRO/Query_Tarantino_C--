#pragma once

#include "tarantino/indexer/commands/IndexResult.hpp"
#include "tarantino/indexer/model/book/HeaderParser.hpp"
#include "tarantino/indexer/model/terms/Tokenizer.hpp"
#include "tarantino/indexer/ports/datamarts/InvertedIndexStorage.hpp"
#include "tarantino/indexer/ports/datamarts/MetadataStorage.hpp"
#include "tarantino/indexer/ports/sources/DatalakeReader.hpp"

#include <memory>
#include <vector>

namespace tarantino::indexer::commands {

/// Indexes books read from the datalake (SPEC §9): saves the metadata of each one, adds its terms to the inverted index
/// and flushes the index once for all of them.
class IndexBookCommand {
public:
    IndexBookCommand(std::shared_ptr<ports::DatalakeReader> datalake, model::HeaderParser headerParser,
                     model::Tokenizer tokenizer, std::shared_ptr<ports::InvertedIndexStorage> invertedIndex,
                     std::shared_ptr<ports::MetadataStorage> metadata);

    IndexResult execute(int bookId);

    /// Indexes the books and flushes the inverted index once for all of them, if any was found.
    std::vector<IndexResult> execute(const std::vector<int>& bookIds);

private:
    IndexResult add(int bookId);

    std::shared_ptr<ports::DatalakeReader> datalake_;
    model::HeaderParser headerParser_;
    model::Tokenizer tokenizer_;
    std::shared_ptr<ports::InvertedIndexStorage> invertedIndex_;
    std::shared_ptr<ports::MetadataStorage> metadata_;
};

}
