#include "tarantino/indexer/IndexerFactory.hpp"
#include "tarantino/indexer/adapters/datalake/batch/BatchBasedDatalakeReader.hpp"
#include "tarantino/indexer/adapters/datalake/book/BookBasedDatalakeReader.hpp"
#include "tarantino/indexer/adapters/datalake/time/TimeBasedDatalakeReader.hpp"
#include "tarantino/indexer/adapters/index/folders/FolderPerTermIndexAdapter.hpp"
#include "tarantino/indexer/adapters/index/json/MonolithicJsonIndexAdapter.hpp"
#include "tarantino/indexer/adapters/metadata/SqliteMetadataAdapter.hpp"

#include <gtest/gtest.h>

namespace tarantino::indexer {
namespace {

IndexerConfig config(const std::string& layout, const std::string& index, const std::string& metadata) {
    return IndexerConfig{"datalake", layout, "datamarts", index, metadata, "mongodb://localhost:27017", "workload"};
}

template <typename Adapter, typename Port>
bool isA(const std::shared_ptr<Port>& port) {
    return std::dynamic_pointer_cast<Adapter>(port) != nullptr;
}

template <typename Function>
std::string rejection(Function function) {
    try {
        function();
    } catch (const std::invalid_argument& error) {
        return error.what();
    }
    return "accepted";
}

TEST(IndexerFactoryTest, selectsTheConfiguredStructures) {
    EXPECT_TRUE(isA<adapters::TimeBasedDatalakeReader>(IndexerFactory::datalakeReader(config("time", "json", "sqlite"))));
    EXPECT_TRUE(isA<adapters::BookBasedDatalakeReader>(IndexerFactory::datalakeReader(config("book", "json", "sqlite"))));
    EXPECT_TRUE(isA<adapters::BatchBasedDatalakeReader>(IndexerFactory::datalakeReader(config("batch", "json", "sqlite"))));
    EXPECT_TRUE(isA<adapters::MonolithicJsonIndexAdapter>(IndexerFactory::invertedIndex(config("time", "json", "sqlite"))));
    EXPECT_TRUE(isA<adapters::FolderPerTermIndexAdapter>(IndexerFactory::invertedIndex(config("time", "folders", "sqlite"))));
    EXPECT_TRUE(isA<adapters::SqliteMetadataAdapter>(IndexerFactory::metadata(config("time", "json", "sqlite"))));
}

TEST(IndexerFactoryTest, rejectsUnknownOptionsNamingThem) {
    EXPECT_EQ("Unknown datalake layout: hash", rejection([] { IndexerFactory::datalakeReader(config("hash", "json", "sqlite")); }));
    EXPECT_EQ("Unknown index structure: trie", rejection([] { IndexerFactory::invertedIndex(config("time", "trie", "sqlite")); }));
    EXPECT_EQ("Unknown metadata backend: postgres", rejection([] { IndexerFactory::metadata(config("time", "json", "postgres")); }));
}

TEST(IndexerFactoryTest, rejectsMongoInABuildWithoutMongoSupport) {
    if (IndexerFactory::withMongo()) {
        GTEST_SKIP() << "this build has MongoDB support";
    }
    EXPECT_NE("accepted", rejection([] { IndexerFactory::invertedIndex(config("time", "mongo", "sqlite")); }));
    EXPECT_NE("accepted", rejection([] { IndexerFactory::metadata(config("time", "json", "mongo")); }));
}

}
}
