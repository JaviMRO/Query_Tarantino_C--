#include "tarantino/query/adapters/index/mongo/MongodbIndexReader.hpp"
#include "tarantino/query/adapters/metadata/MongodbMetadataReader.hpp"
#include "tarantino/query/adapters/mongo/MongoDatabases.hpp"

#include "indexer/tests/adapters/mongo/TemporaryMongoDatabase.hpp"

#include <bsoncxx/builder/basic/array.hpp>
#include <bsoncxx/builder/basic/document.hpp>
#include <bsoncxx/builder/basic/kvp.hpp>
#include <bsoncxx/types.hpp>
#include <gtest/gtest.h>

namespace tarantino::query::adapters {
namespace {

using bsoncxx::builder::basic::kvp;
using bsoncxx::builder::basic::make_array;
using bsoncxx::builder::basic::make_document;
using indexer::adapters::TemporaryMongoDatabase;
using model::BookMetadata;

bsoncxx::document::value book(int bookId, const std::string& title, const std::optional<std::string>& author) {
    bsoncxx::builder::basic::document document;
    document.append(kvp("book_id", bookId), kvp("title", title));
    if (author) {
        document.append(kvp("author", *author));
    } else {
        document.append(kvp("author", bsoncxx::types::b_null{}));
    }
    document.append(kvp("language", "English"), kvp("path", "datalake/" + std::to_string(bookId) + "/body.txt"));
    return document.extract();
}

class MongodbReadersTest : public ::testing::Test {
protected:
    void SetUp() override {
        uri = mongo.uri();
        if (!uri) {
            GTEST_SKIP() << TemporaryMongoDatabase::unavailable();
        }
        auto database = MongoDatabases::database(*uri);
        database["inverted_index"].insert_one(make_document(kvp("term", "island"), kvp("postings", make_array(1342, 5))));
        std::vector<bsoncxx::document::value> books{book(76, "Huckleberry Finn", "Mark Twain"),
                                                    book(74, "Tom Sawyer", "Mark Twain"), book(84, "Frankenstein", std::nullopt)};
        database["books"].insert_many(books);
    }

    TemporaryMongoDatabase mongo;
    std::optional<std::string> uri;
};

TEST_F(MongodbReadersTest, readsPostingsOfATermAndNothingForUnknownTerms) {
    MongodbIndexReader index(*uri);

    EXPECT_EQ((std::unordered_set<int>{5, 1342}), index.postings("island").ids());
    EXPECT_TRUE(index.postings("whale").ids().empty());
}

TEST_F(MongodbReadersTest, findsABookByIdKeepingMissingFieldsEmpty) {
    EXPECT_EQ((BookMetadata{84, "Frankenstein", std::nullopt, "English", "datalake/84/body.txt"}),
              MongodbMetadataReader(*uri).book(84));
}

TEST_F(MongodbReadersTest, findsBooksByCaseInsensitiveAuthorSubstringOrderedById) {
    std::vector<int> ids;
    for (const BookMetadata& found : MongodbMetadataReader(*uri).booksBy("twain")) {
        ids.push_back(found.bookId);
    }

    EXPECT_EQ((std::vector<int>{74, 76}), ids);
}

TEST(MongodbMetadataReaderTest, escapesEveryRegularExpressionMetacharacter) {
    EXPECT_EQ("Smith \\(ed\\.\\) \\[1\\]\\+\\*\\?\\|\\^\\$\\{2\\}\\\\", MongodbMetadataReader::quoted("Smith (ed.) [1]+*?|^${2}\\"));
}

}
}
