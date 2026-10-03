#include "tarantino/indexer/adapters/mongo/MongoDatabases.hpp"

#include "indexer/tests/adapters/mongo/TemporaryMongoDatabase.hpp"

#include <gtest/gtest.h>

namespace tarantino::indexer::adapters {
namespace {

TEST(MongoDatabasesTest, useTheUriDatabaseOrDefaultToTarantino) {
    EXPECT_EQ("tarantino", MongoDatabases::databaseName("mongodb://localhost:27017"));
    EXPECT_EQ("tarantino_benchmark", MongoDatabases::databaseName("mongodb://localhost:27017/tarantino_benchmark"));
    EXPECT_EQ("tarantino", std::string(MongoDatabases::database("mongodb://localhost:27017").name()));
}

TEST(MongoDatabasesTest, putsAnotherDatabaseInAConnectionStringKeepingItsOptions) {
    EXPECT_EQ("mongodb://localhost:27017/test", TemporaryMongoDatabase::withDatabase("mongodb://localhost:27017", "test"));
    EXPECT_EQ("mongodb://h:1/test?w=1", TemporaryMongoDatabase::withDatabase("mongodb://h:1/other?w=1", "test"));
    EXPECT_EQ("mongodb://h:1/test?w=1", TemporaryMongoDatabase::withDatabase("mongodb://h:1/?w=1", "test"));
}

}
}
