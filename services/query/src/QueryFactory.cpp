#include "tarantino/query/QueryFactory.hpp"

#include "tarantino/query/adapters/index/folders/FolderPerTermIndexReader.hpp"
#include "tarantino/query/adapters/index/json/MonolithicJsonIndexReader.hpp"
#include "tarantino/query/adapters/metadata/SqliteMetadataReader.hpp"
#include "tarantino/query/adapters/stopwords/FileStopwordsLoader.hpp"

#ifdef TARANTINO_WITH_MONGO
#include "tarantino/query/adapters/index/mongo/MongodbIndexReader.hpp"
#include "tarantino/query/adapters/metadata/MongodbMetadataReader.hpp"
#endif

#include <stdexcept>
#include <string>

namespace tarantino::query {

namespace {

constexpr const char* MONGO = "mongo";

[[noreturn]] void unknown(const std::string& kind, const std::string& name) {
    std::string message = "Unknown " + kind + ": " + name;
    if (name == MONGO) {
        message += " (this build has no MongoDB support: configure it with -DTARANTINO_MONGO=FETCH or install mongocxx)";
    }
    throw std::invalid_argument(message);
}

}

commands::SearchCommand QueryFactory::searchCommand(const QueryConfig& config) {
    auto index = invertedIndex(config);
    auto books = metadata(config);
    return commands::SearchCommand(std::move(index), std::move(books), stopwords(config));
}

std::shared_ptr<ports::InvertedIndexReader> QueryFactory::invertedIndex(const QueryConfig& config) {
    if (config.index == "json") {
        return std::make_shared<adapters::MonolithicJsonIndexReader>(config.datamarts / "inverted_index.json");
    }
    if (config.index == "folders") {
        return std::make_shared<adapters::FolderPerTermIndexReader>(config.datamarts / "inverted_index");
    }
#ifdef TARANTINO_WITH_MONGO
    if (config.index == MONGO) {
        return std::make_shared<adapters::MongodbIndexReader>(config.mongoUri);
    }
#endif
    unknown("index structure", config.index);
}

std::shared_ptr<ports::MetadataReader> QueryFactory::metadata(const QueryConfig& config) {
    if (config.metadata == "sqlite") {
        return std::make_shared<adapters::SqliteMetadataReader>(config.datamarts / "metadata.db");
    }
#ifdef TARANTINO_WITH_MONGO
    if (config.metadata == MONGO) {
        return std::make_shared<adapters::MongodbMetadataReader>(config.mongoUri);
    }
#endif
    unknown("metadata backend", config.metadata);
}

platform::StringSet QueryFactory::stopwords(const QueryConfig& config) {
    return adapters::FileStopwordsLoader(config.workload / "stopwords.txt").stopwords();
}

bool QueryFactory::withMongo() {
#ifdef TARANTINO_WITH_MONGO
    return true;
#else
    return false;
#endif
}

}
