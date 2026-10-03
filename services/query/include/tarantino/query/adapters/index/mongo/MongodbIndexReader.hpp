#pragma once

#include "tarantino/query/ports/InvertedIndexReader.hpp"

#include <mongocxx/collection.hpp>

namespace tarantino::query::adapters {

/// Reads the mongo structure (SPEC §8.1): one find by term on every lookup, reading the whole document; a missing
/// document is no postings. Postings are read as a set.
class MongodbIndexReader : public ports::InvertedIndexReader {
public:
    explicit MongodbIndexReader(const std::string& connectionUri);

    Postings postings(const std::string& term) override;

private:
    mongocxx::collection collection_;
};

}
