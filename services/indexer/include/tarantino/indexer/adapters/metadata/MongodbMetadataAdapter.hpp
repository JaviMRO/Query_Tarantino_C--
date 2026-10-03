#pragma once

#include "tarantino/indexer/ports/datamarts/MetadataStorage.hpp"

#include <mongocxx/collection.hpp>

#include <string>

namespace tarantino::indexer::adapters {

/// The mongo metadata backend (SPEC §8.2): collection books, one document per book with a unique index on book_id,
/// created when it is opened; a save is one replaceOne by book_id with upsert.
class MongodbMetadataAdapter : public ports::MetadataStorage {
public:
    explicit MongodbMetadataAdapter(const std::string& connectionUri);

    void save(const model::Book& book) override;

private:
    mongocxx::collection collection_;
};

}
