#include "tarantino/indexer/commands/IndexBookCommand.hpp"

#include <algorithm>

namespace tarantino::indexer::commands {

IndexBookCommand::IndexBookCommand(std::shared_ptr<ports::DatalakeReader> datalake, model::HeaderParser headerParser,
                                   model::Tokenizer tokenizer, std::shared_ptr<ports::InvertedIndexStorage> invertedIndex,
                                   std::shared_ptr<ports::MetadataStorage> metadata)
    : datalake_(std::move(datalake)),
      headerParser_(headerParser),
      tokenizer_(std::move(tokenizer)),
      invertedIndex_(std::move(invertedIndex)),
      metadata_(std::move(metadata)) {
}

IndexResult IndexBookCommand::execute(int bookId) {
    return execute(std::vector<int>{bookId}).front();
}

std::vector<IndexResult> IndexBookCommand::execute(const std::vector<int>& bookIds) {
    std::vector<IndexResult> results;
    results.reserve(bookIds.size());
    for (const int bookId : bookIds) {
        results.push_back(add(bookId));
    }
    if (std::any_of(results.begin(), results.end(), [](const IndexResult& result) { return result.indexed; })) {
        invertedIndex_->flush();
    }
    return results;
}

IndexResult IndexBookCommand::add(int bookId) {
    const auto text = datalake_->bookText(bookId);
    if (!text) {
        return IndexResult::notFound(bookId);
    }
    metadata_->save(headerParser_.book(*text));
    const model::TermOccurrences occurrences = tokenizer_.occurrences(text->bookId, text->body);
    invertedIndex_->add(occurrences);
    return IndexResult::success(text->bookId, static_cast<int>(occurrences.frequencies.size()));
}

}
