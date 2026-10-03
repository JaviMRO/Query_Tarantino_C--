#include "tarantino/indexer/IndexerFactory.hpp"

#include "tarantino/indexer/adapters/datalake/batch/BatchBasedDatalakeReader.hpp"
#include "tarantino/indexer/adapters/datalake/book/BookBasedDatalakeReader.hpp"
#include "tarantino/indexer/adapters/datalake/time/TimeBasedDatalakeReader.hpp"
#include "tarantino/indexer/adapters/index/folders/FolderPerTermIndexAdapter.hpp"
#include "tarantino/indexer/adapters/index/json/MonolithicJsonIndexAdapter.hpp"
#include "tarantino/indexer/adapters/metadata/SqliteMetadataAdapter.hpp"
#include "tarantino/indexer/adapters/stopwords/FileStopwordsLoader.hpp"

#ifdef TARANTINO_WITH_MONGO
#include "tarantino/indexer/adapters/index/mongo/MongodbIndexAdapter.hpp"
#include "tarantino/indexer/adapters/metadata/MongodbMetadataAdapter.hpp"
#endif

#include <stdexcept>
#include <string>

namespace tarantino::indexer {

namespace {

constexpr const char* MONGO = "mongo";

[[noreturn]] void unknown(const std::string& kind, const std::string& name) {
    std::string message = "Unknown " + kind + ": " + name;
    if (name == MONGO) {
        message += " (this build has no MongoDB support: configure it with -DTARANTINO_MONGO=FETCH or install mongocxx)";
    }
    throw std::invalid_argument(message);
}

model::Tokenizer tokenizer(const IndexerConfig& config) {
    return model::Tokenizer(adapters::FileStopwordsLoader(config.workload / "stopwords.txt").stopwords());
}

}

commands::IndexBookCommand IndexerFactory::indexCommand(const IndexerConfig& config) {
    auto datalake = datalakeReader(config);
    model::Tokenizer terms = tokenizer(config);
    auto index = invertedIndex(config);
    return commands::IndexBookCommand(std::move(datalake), model::HeaderParser(), std::move(terms), std::move(index),
                                      metadata(config));
}

std::shared_ptr<ports::DatalakeReader> IndexerFactory::datalakeReader(const IndexerConfig& config) {
    if (config.datalakeLayout == "time") {
        return std::make_shared<adapters::TimeBasedDatalakeReader>(config.datalake);
    }
    if (config.datalakeLayout == "book") {
        return std::make_shared<adapters::BookBasedDatalakeReader>(config.datalake);
    }
    if (config.datalakeLayout == "batch") {
        return std::make_shared<adapters::BatchBasedDatalakeReader>(config.datalake);
    }
    unknown("datalake layout", config.datalakeLayout);
}

std::shared_ptr<ports::InvertedIndexStorage> IndexerFactory::invertedIndex(const IndexerConfig& config) {
    if (config.index == "json") {
        return std::make_shared<adapters::MonolithicJsonIndexAdapter>(config.datamarts / "inverted_index.json");
    }
    if (config.index == "folders") {
        return std::make_shared<adapters::FolderPerTermIndexAdapter>(config.datamarts / "inverted_index");
    }
#ifdef TARANTINO_WITH_MONGO
    if (config.index == MONGO) {
        return std::make_shared<adapters::MongodbIndexAdapter>(config.mongoUri);
    }
#endif
    unknown("index structure", config.index);
}

std::shared_ptr<ports::MetadataStorage> IndexerFactory::metadata(const IndexerConfig& config) {
    if (config.metadata == "sqlite") {
        return std::make_shared<adapters::SqliteMetadataAdapter>(config.datamarts / "metadata.db");
    }
#ifdef TARANTINO_WITH_MONGO
    if (config.metadata == MONGO) {
        return std::make_shared<adapters::MongodbMetadataAdapter>(config.mongoUri);
    }
#endif
    unknown("metadata backend", config.metadata);
}

bool IndexerFactory::withMongo() {
#ifdef TARANTINO_WITH_MONGO
    return true;
#else
    return false;
#endif
}

}
