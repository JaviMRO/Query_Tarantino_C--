#pragma once

#include "tarantino/indexer/model/book/Book.hpp"

namespace tarantino::indexer::ports {

/// The metadata datamart of SPEC §8.2. A save replaces the book if it exists and is committed before it returns.
class MetadataStorage {
public:
    virtual ~MetadataStorage() = default;

    virtual void save(const model::Book& book) = 0;
};

}
