#include "tarantino/query/QueryFactory.hpp"
#include "tarantino/query/adapters/index/folders/FolderPerTermIndexReader.hpp"
#include "tarantino/query/adapters/index/json/MonolithicJsonIndexReader.hpp"
#include "tarantino/query/adapters/metadata/SqliteMetadataReader.hpp"

#include <gtest/gtest.h>

namespace tarantino::query {
namespace {

QueryConfig config(const std::string& index, const std::string& metadata) {
    return QueryConfig{"datamarts", index, metadata, "mongodb://localhost:27017", "workload"};
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

TEST(QueryFactoryTest, selectsTheConfiguredStructures) {
    EXPECT_TRUE(isA<adapters::MonolithicJsonIndexReader>(QueryFactory::invertedIndex(config("json", "sqlite"))));
    EXPECT_TRUE(isA<adapters::FolderPerTermIndexReader>(QueryFactory::invertedIndex(config("folders", "sqlite"))));
    EXPECT_TRUE(isA<adapters::SqliteMetadataReader>(QueryFactory::metadata(config("json", "sqlite"))));
}

TEST(QueryFactoryTest, rejectsUnknownOptionsNamingThem) {
    EXPECT_EQ("Unknown index structure: trie", rejection([] { QueryFactory::invertedIndex(config("trie", "sqlite")); }));
    EXPECT_EQ("Unknown metadata backend: postgres", rejection([] { QueryFactory::metadata(config("json", "postgres")); }));
}

}
}
