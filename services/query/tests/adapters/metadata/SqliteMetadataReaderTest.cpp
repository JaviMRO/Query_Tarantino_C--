#include "tarantino/query/adapters/metadata/SqliteMetadataReader.hpp"
#include "tarantino/platform/Paths.hpp"
#include "tarantino/testing/TempDirectory.hpp"

#include <gtest/gtest.h>
#include <sqlite3.h>

namespace tarantino::query::adapters {
namespace {

using model::BookMetadata;

class SqliteMetadataReaderTest : public ::testing::Test {
protected:
    void SetUp() override {
        execute("CREATE TABLE books (book_id INTEGER PRIMARY KEY, title TEXT, author TEXT, language TEXT, path TEXT NOT NULL)");
        execute("INSERT INTO books VALUES (76, 'Huckleberry Finn', 'Mark Twain', 'English', 'datalake/76/body.txt')");
        execute("INSERT INTO books VALUES (74, 'Tom Sawyer', 'Mark Twain', 'English', 'datalake/74/body.txt')");
        execute("INSERT INTO books VALUES (84, 'Frankenstein', 'Mary Shelley', 'English', 'datalake/84/body.txt')");
    }

    void execute(const std::string& sql) {
        sqlite3* connection = nullptr;
        sqlite3_open(platform::portable(database).c_str(), &connection);
        ASSERT_EQ(SQLITE_OK, sqlite3_exec(connection, sql.c_str(), nullptr, nullptr, nullptr)) << sqlite3_errmsg(connection);
        sqlite3_close(connection);
    }

    static std::vector<int> ids(const std::vector<BookMetadata>& books) {
        std::vector<int> found;
        for (const BookMetadata& book : books) {
            found.push_back(book.bookId);
        }
        return found;
    }

    testing::TempDirectory directory;
    std::filesystem::path database = directory / "metadata.db";
};

TEST_F(SqliteMetadataReaderTest, findsABookById) {
    EXPECT_EQ((BookMetadata{84, "Frankenstein", "Mary Shelley", "English", "datalake/84/body.txt"}),
              SqliteMetadataReader(database).book(84));
    EXPECT_EQ(std::nullopt, SqliteMetadataReader(database).book(1));
}

TEST_F(SqliteMetadataReaderTest, findsBooksByCaseInsensitiveAuthorSubstringOrderedById) {
    EXPECT_EQ((std::vector<int>{74, 76}), ids(SqliteMetadataReader(database).booksBy("twain")));
}

TEST_F(SqliteMetadataReaderTest, matchesPercentUnderscoreAndBackslashLiterally) {
    execute("INSERT INTO books VALUES (1, 'One', 'A_B\\C', 'English', 'datalake/1/body.txt')");
    execute("INSERT INTO books VALUES (2, 'Two', 'AxB', 'English', 'datalake/2/body.txt')");
    SqliteMetadataReader reader(database);

    EXPECT_EQ(std::vector<int>{1}, ids(reader.booksBy("a_b\\c")));
    EXPECT_EQ(std::vector<int>{1}, ids(reader.booksBy("a_b")));
    EXPECT_TRUE(reader.booksBy("%").empty());
}

TEST_F(SqliteMetadataReaderTest, keepsItsConnectionAndSeesBooksSavedAfterItsFirstQuery) {
    SqliteMetadataReader reader(database);
    EXPECT_EQ(std::nullopt, reader.book(1342));
    execute("INSERT INTO books VALUES (1342, 'Pride and Prejudice', 'Jane Austen', 'English', 'datalake/1342/body.txt')");

    const auto book = reader.book(1342);

    ASSERT_TRUE(book.has_value());
    EXPECT_EQ("Pride and Prejudice", book->title);
}

TEST_F(SqliteMetadataReaderTest, keepsMissingFieldsEmpty) {
    execute("INSERT INTO books VALUES (7, NULL, NULL, NULL, '7.body.txt')");

    EXPECT_EQ((BookMetadata{7, std::nullopt, std::nullopt, std::nullopt, "7.body.txt"}), SqliteMetadataReader(database).book(7));
}

TEST_F(SqliteMetadataReaderTest, treatsAMissingDatabaseAsEmptyWithoutCreatingIt) {
    SqliteMetadataReader reader(directory / "missing.db");

    EXPECT_EQ(std::nullopt, reader.book(84));
    EXPECT_TRUE(reader.booksBy("Shelley").empty());
    EXPECT_FALSE(std::filesystem::exists(directory / "missing.db"));
}

}
}
