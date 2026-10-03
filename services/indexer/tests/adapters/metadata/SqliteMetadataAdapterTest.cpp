#include "tarantino/indexer/adapters/metadata/SqliteMetadataAdapter.hpp"
#include "tarantino/platform/Paths.hpp"
#include "tarantino/testing/TempDirectory.hpp"

#include <gtest/gtest.h>
#include <sqlite3.h>

namespace tarantino::indexer::adapters {
namespace {

using model::Book;
using Row = std::vector<std::optional<std::string>>;

std::vector<Row> rows(const std::filesystem::path& database) {
    sqlite3* connection = nullptr;
    sqlite3_open(platform::portable(database).c_str(), &connection);
    sqlite3_stmt* statement = nullptr;
    sqlite3_prepare_v2(connection, "SELECT * FROM books ORDER BY book_id", -1, &statement, nullptr);
    std::vector<Row> result;
    while (sqlite3_step(statement) == SQLITE_ROW) {
        Row row;
        for (int column = 0; column < 5; ++column) {
            const auto* text = sqlite3_column_text(statement, column);
            row.push_back(text == nullptr ? std::nullopt : std::optional<std::string>(reinterpret_cast<const char*>(text)));
        }
        result.push_back(row);
    }
    sqlite3_finalize(statement);
    sqlite3_close(connection);
    return result;
}

class SqliteMetadataAdapterTest : public ::testing::Test {
protected:
    testing::TempDirectory directory;
};

TEST_F(SqliteMetadataAdapterTest, createsTheDatabaseAndUpsertsBooks) {
    const std::filesystem::path database = directory / "datamarts/metadata.db";
    const std::filesystem::path body = std::filesystem::path("datalake") / "5" / "body.txt";
    {
        SqliteMetadataAdapter metadata(database);
        metadata.save(Book{5, "Old title", std::nullopt, "English", body});
        metadata.save(Book{5, "Robinson Crusoe", "Daniel Defoe", "English", body});
    }

    EXPECT_EQ((std::vector<Row>{{"5", "Robinson Crusoe", "Daniel Defoe", "English", "datalake/5/body.txt"}}), rows(database));
}

TEST_F(SqliteMetadataAdapterTest, storesMissingFieldsAsNull) {
    const std::filesystem::path database = directory / "metadata.db";
    {
        SqliteMetadataAdapter(database).save(Book{7, std::nullopt, std::nullopt, std::nullopt, "7.body.txt"});
    }

    EXPECT_EQ((Row{"7", std::nullopt, std::nullopt, std::nullopt, "7.body.txt"}), rows(database).front());
}

TEST_F(SqliteMetadataAdapterTest, commitsEverySaveBeforeItReturns) {
    const std::filesystem::path database = directory / "metadata.db";
    SqliteMetadataAdapter metadata(database);
    metadata.save(Book{7, "Seven", std::nullopt, std::nullopt, "7.body.txt"});

    // Another connection sees the book while this one is still open
    EXPECT_EQ(1u, rows(database).size());
}

}
}
