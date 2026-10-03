#pragma once

#include <cstdint>
#include <string>
#include <unordered_set>

namespace tarantino::indexer::benchmarking {

/// The MongoDB databases of the benchmarks, one per benchmark (SPEC §11), each reached with a client of its own.
class MongoStores {
public:
    MongoStores() = delete;

    static void drop(const std::string& connectionUri);

    /// Copies every collection of the source database to the target, with its indexes.
    static void copy(const std::string& sourceUri, const std::string& targetUri);

    /// Puts back the documents of some values of a unique field as the snapshot has them, deleting the others.
    static void restoreDocuments(const std::string& snapshotUri, const std::string& targetUri, const std::string& collection,
                                 const std::string& field, const std::unordered_set<std::string>& values);

    static std::uint64_t documentCount(const std::string& connectionUri, const std::string& collection);

    /// The storageSize and totalIndexSize of every collection after an fsync: storage already allocated (SPEC §11).
    static std::uint64_t diskUsage(const std::string& connectionUri);

    /// The version of the server, which the environment of a run records (SPEC §11), or why it is unknown.
    static std::string serverVersion(const std::string& connectionUri);
};

}
