#pragma once

#include "tarantino/indexer/model/terms/TermOccurrences.hpp"

namespace tarantino::indexer::ports {

/// The inverted index of SPEC §8.1, in one of its structures. What is added is written by the next flush.
class InvertedIndexStorage {
public:
    virtual ~InvertedIndexStorage() = default;

    /// Loads from storage whatever the structure keeps in memory, before the first book is added. Adding loads it
    /// anyway; opening first lets the cost of loading be paid, or measured, apart from indexing.
    virtual void open() {
    }

    virtual void add(const model::TermOccurrences& occurrences) = 0;

    virtual void flush() = 0;
};

}
