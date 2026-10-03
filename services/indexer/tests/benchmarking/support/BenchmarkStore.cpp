#include "indexer/tests/benchmarking/support/BenchmarkStore.hpp"

#include "crawler/tests/benchmarking/support/environment/BenchmarkPaths.hpp"
#include "crawler/tests/benchmarking/support/files/Directories.hpp"
#include "indexer/tests/adapters/index/folders/TermFileRestore.hpp"
#include "tarantino/indexer/IndexerFactory.hpp"
#include "tarantino/platform/Files.hpp"

#ifdef TARANTINO_WITH_MONGO
#include "indexer/tests/benchmarking/support/MongoStores.hpp"
#endif

#include <nlohmann/json.hpp>

#include <algorithm>
#include <stdexcept>

namespace tarantino::indexer::benchmarking {

using crawler::benchmarking::BenchmarkPaths;
using crawler::benchmarking::Directories;

namespace {

constexpr const char* MONGO = "mongo";
constexpr const char* FOLDERS = "folders";
constexpr const char* INVERTED_INDEX = "inverted_index";

IndexerConfig configOf(const std::string& name, const std::string& index, const std::string& metadata) {
    std::string database = name;
    std::replace(database.begin(), database.end(), '-', '_');
    return IndexerConfig{"datalake", "time", BenchmarkPaths::scratch(name), index, metadata, BenchmarkPaths::mongoUri(database),
                         BenchmarkPaths::workload()};
}

#ifndef TARANTINO_WITH_MONGO
[[noreturn]] void withoutMongo() {
    throw std::logic_error("This build has no MongoDB support");
}
#endif

}

BenchmarkStore::BenchmarkStore(IndexerConfig config, bool mongo) : config_(std::move(config)), mongo_(mongo) {
}

BenchmarkStore BenchmarkStore::forIndex(const std::string& index, const std::string& name) {
    return BenchmarkStore(configOf(name, index, "sqlite"), index == MONGO);
}

BenchmarkStore BenchmarkStore::forMetadata(const std::string& metadata, const std::string& name) {
    return BenchmarkStore(configOf(name, "json", metadata), metadata == MONGO);
}

std::shared_ptr<ports::InvertedIndexStorage> BenchmarkStore::invertedIndex() const {
    return IndexerFactory::invertedIndex(config_);
}

std::shared_ptr<ports::MetadataStorage> BenchmarkStore::metadata() const {
    return IndexerFactory::metadata(config_);
}

const std::filesystem::path& BenchmarkStore::datamarts() const {
    return config_.datamarts;
}

const std::string& BenchmarkStore::mongoUri() const {
    return config_.mongoUri;
}

const IndexerConfig& BenchmarkStore::config() const {
    return config_;
}

void BenchmarkStore::clear() const {
    Directories::remove(config_.datamarts);
#ifdef TARANTINO_WITH_MONGO
    if (mongo_) {
        MongoStores::drop(config_.mongoUri);
    }
#endif
}

void BenchmarkStore::copyTo(const BenchmarkStore& target) const {
    target.clear();
    Directories::copy(config_.datamarts, target.config_.datamarts);
    if (mongo_) {
#ifdef TARANTINO_WITH_MONGO
        MongoStores::copy(config_.mongoUri, target.config_.mongoUri);
#else
        withoutMongo();
#endif
    }
}

void BenchmarkStore::restoreTermsFrom(const BenchmarkStore& snapshot, const std::unordered_set<std::string>& touchedTerms) const {
    if (config_.index == FOLDERS) {
        adapters::TermFileRestore::restore(snapshot.folderIndex(), folderIndex(), touchedTerms);
    } else if (config_.index == MONGO) {
#ifdef TARANTINO_WITH_MONGO
        MongoStores::restoreDocuments(snapshot.config_.mongoUri, config_.mongoUri, INVERTED_INDEX, "term", touchedTerms);
#else
        withoutMongo();
#endif
    } else {
        snapshot.copyTo(*this);
    }
}

StoreFootprint BenchmarkStore::footprint() const {
    if (mongo_) {
#ifdef TARANTINO_WITH_MONGO
        const std::uint64_t bytes = MongoStores::diskUsage(config_.mongoUri);
        return StoreFootprint{bytes, bytes, MongoStores::documentCount(config_.mongoUri, INVERTED_INDEX)};
#else
        withoutMongo();
#endif
    }
    if (config_.index == FOLDERS) {
        const crawler::benchmarking::Footprint folders = Directories::footprint(folderIndex());
        return StoreFootprint{folders.bytes, folders.allocatedBytes, folders.files};
    }
    const crawler::benchmarking::Footprint json = Directories::footprint(config_.datamarts);
    const nlohmann::json index = nlohmann::json::parse(platform::files::readString(config_.datamarts / "inverted_index.json"));
    return StoreFootprint{json.bytes, json.allocatedBytes, index.size()};
}

std::filesystem::path BenchmarkStore::folderIndex() const {
    return config_.datamarts / INVERTED_INDEX;
}

}
