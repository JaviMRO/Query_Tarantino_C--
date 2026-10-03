#pragma once

#include "tarantino/query/ports/MetadataReader.hpp"

#include <filesystem>
#include <map>
#include <string>

struct sqlite3;
struct sqlite3_stmt;

namespace tarantino::query::adapters {

/// Reads the sqlite metadata backend (SPEC §8.2) through SQLite's C API: one connection, opened by the first query, and
/// one prepared statement per query, bound and reset on every call. A missing metadata.db has no books and is not
/// created.
class SqliteMetadataReader : public ports::MetadataReader {
public:
    explicit SqliteMetadataReader(std::filesystem::path database);

    SqliteMetadataReader(const SqliteMetadataReader&) = delete;
    SqliteMetadataReader& operator=(const SqliteMetadataReader&) = delete;

    ~SqliteMetadataReader() override;

    std::optional<model::BookMetadata> book(int bookId) override;

    /// SELECT … WHERE author LIKE '%<author>%' ESCAPE '\' ORDER BY book_id, with \, % and _ of the author escaped so
    /// that they match literally: SQLite's LIKE folds ASCII letters only (SPEC §12).
    std::vector<model::BookMetadata> booksBy(const std::string& author) override;

private:
    sqlite3_stmt* statement(const char* sql);

    std::vector<model::BookMetadata> books(sqlite3_stmt* statement);

    bool available();

    std::filesystem::path database_;
    sqlite3* connection_ = nullptr;
    // By the address of the constant that holds the SQL, so that a lookup copies nothing
    std::map<const char*, sqlite3_stmt*> statements_;
};

}
