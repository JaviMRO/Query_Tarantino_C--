#pragma once

#include "tarantino/indexer/adapters/index/PendingPostings.hpp"
#include "tarantino/indexer/ports/datamarts/InvertedIndexStorage.hpp"

#include <mongocxx/collection.hpp>

#include <string>

namespace tarantino::indexer::adapters {

/// The mongo structure (SPEC §8.1): one document per term, {"term": t, "postings": [ids]}, with a unique index on term
/// created when it is opened. A flush sends one unordered bulk write with an upsert per term that adds its new ids
/// with $addToSet, never one request per term.
class MongodbIndexAdapter : public ports::InvertedIndexStorage {
public:
    explicit MongodbIndexAdapter(const std::string& connectionUri);

    void add(const model::TermOccurrences& occurrences) override;

    void flush() override;

private:
    mongocxx::collection collection_;
    PendingPostings pending_;
};

}
