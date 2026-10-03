#pragma once

#include "indexer/tests/benchmarking/support/StoreFootprint.hpp"
#include "tarantino/indexer/IndexerConfig.hpp"
#include "tarantino/indexer/ports/datamarts/InvertedIndexStorage.hpp"
#include "tarantino/indexer/ports/datamarts/MetadataStorage.hpp"

#include <filesystem>
#include <memory>
#include <string>
#include <unordered_set>

namespace tarantino::indexer::benchmarking {

/// The datamarts of a benchmark: a directory of the scratch directory and, for mongo, a database of its own.
class BenchmarkStore {
public:
    static BenchmarkStore forIndex(const std::string& index, const std::string& name);

    static BenchmarkStore forMetadata(const std::string& metadata, const std::string& name);

    std::shared_ptr<ports::InvertedIndexStorage> invertedIndex() const;

    std::shared_ptr<ports::MetadataStorage> metadata() const;

    const std::filesystem::path& datamarts() const;

    const std::string& mongoUri() const;

    const IndexerConfig& config() const;

    void clear() const;

    void copyTo(const BenchmarkStore& target) const;

    /// Restores the snapshot after an update that only touched the given terms. For folders and MongoDB only their
    /// files or documents are put back, instead of hundreds of thousands of them; json, a single file, is copied.
    void restoreTermsFrom(const BenchmarkStore& snapshot, const std::unordered_set<std::string>& touchedTerms) const;

    /// Measures the index once it is flushed. MongoDB reports allocated storage, so its two sizes are equal; a folders
    /// index is walked once for its files, sizes and blocks.
    StoreFootprint footprint() const;

private:
    BenchmarkStore(IndexerConfig config, bool mongo);

    std::filesystem::path folderIndex() const;

    IndexerConfig config_;
    bool mongo_;
};

}
