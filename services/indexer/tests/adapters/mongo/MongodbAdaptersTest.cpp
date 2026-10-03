#include "tarantino/indexer/adapters/index/mongo/MongodbIndexAdapter.hpp"
#include "tarantino/indexer/adapters/metadata/MongodbMetadataAdapter.hpp"
#include "tarantino/indexer/adapters/mongo/MongoDatabases.hpp"

#include "indexer/tests/adapters/mongo/TemporaryMongoDatabase.hpp"

#include <bsoncxx/builder/basic/document.hpp>
#include <bsoncxx/builder/basic/kvp.hpp>
#include <gtest/gtest.h>

#include <algorithm>

namespace tarantino::indexer::adapters {
namespace {

using bsoncxx::builder::basic::kvp;
using bsoncxx::builder::basic::make_document;
using model::Book;
using model::TermOccurrences;

class MongodbAdaptersTest : public ::testing::Test {
protected:
    void SetUp() override {
        uri = mongo.uri();
        if (!uri) {
            GTEST_SKIP() << TemporaryMongoDatabase::unavailable();
        }
    }

    std::vector<int> postings(const std::string& term) {
        const auto document = MongoDatabases::database(*uri)["inverted_index"].find_one(make_document(kvp("term", term)));
        std::vector<int> ids;
        for (const auto& id : document->view()["postings"].get_array().value) {
            ids.push_back(id.get_int32().value);
        }
        std::sort(ids.begin(), ids.end());
        return ids;
    }

    TemporaryMongoDatabase mongo;
    std::optional<std::string> uri;
};

TEST_F(MongodbAdaptersTest, indexAddsUniquePostingsPerTerm) {
    MongodbIndexAdapter index(*uri);
    index.add(TermOccurrences{1342, {{"island", 2}, {"whale", 1}}});
    index.add(TermOccurrences{5, {{"island", 1}}});
    index.flush();
    index.add(TermOccurrences{5, {{"island", 1}}});
    index.flush();

    EXPECT_EQ((std::vector<int>{5, 1342}), postings("island"));
    EXPECT_EQ((std::vector<int>{1342}), postings("whale"));
}

TEST_F(MongodbAdaptersTest, flushWithoutPendingTermsWritesNothing) {
    MongodbIndexAdapter(*uri).flush();

    EXPECT_EQ(0, MongoDatabases::database(*uri)["inverted_index"].count_documents({}));
}

TEST_F(MongodbAdaptersTest, metadataIsUpsertedByBookId) {
    MongodbMetadataAdapter metadata(*uri);
    const std::filesystem::path body = std::filesystem::path("datalake") / "5" / "body.txt";
    metadata.save(Book{5, "Old title", std::nullopt, "English", body});
    metadata.save(Book{5, "Robinson Crusoe", std::nullopt, "English", body});

    auto books = MongoDatabases::database(*uri)["books"];
    const auto book = books.find_one(make_document(kvp("book_id", 5)));
    ASSERT_TRUE(book.has_value());
    EXPECT_EQ(1, books.count_documents({}));
    EXPECT_EQ("Robinson Crusoe", std::string(book->view()["title"].get_string().value));
    EXPECT_EQ(bsoncxx::type::k_null, book->view()["author"].type());
    EXPECT_EQ("datalake/5/body.txt", std::string(book->view()["path"].get_string().value));
}

}
}
