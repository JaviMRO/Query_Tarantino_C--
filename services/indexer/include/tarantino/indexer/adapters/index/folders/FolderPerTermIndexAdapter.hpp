#pragma once

#include "tarantino/indexer/adapters/index/PendingPostings.hpp"
#include "tarantino/indexer/ports/datamarts/InvertedIndexStorage.hpp"

#include <filesystem>

namespace tarantino::indexer::adapters {

/// The folders structure (SPEC §8.1): one file per term, <root>/<c>/<name>.txt, with one id per line in ascending
/// order. A flush rewrites, atomically, every term file that gains an id with the union of its ids and the new ones.
class FolderPerTermIndexAdapter : public ports::InvertedIndexStorage {
public:
    explicit FolderPerTermIndexAdapter(std::filesystem::path root);

    void add(const model::TermOccurrences& occurrences) override;

    /// Terms come in order, so the files of each folder are written together; each folder is created once.
    void flush() override;

private:
    std::filesystem::path root_;
    PendingPostings pending_;
};

}
