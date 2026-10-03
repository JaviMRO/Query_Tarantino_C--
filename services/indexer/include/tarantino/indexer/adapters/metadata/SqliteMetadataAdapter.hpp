#pragma once

#include "tarantino/indexer/ports/datamarts/MetadataStorage.hpp"

#include <filesystem>

struct sqlite3;
struct sqlite3_stmt;

namespace tarantino::indexer::adapters {

/// The sqlite metadata backend (SPEC §8.2): table books of <datamarts>/metadata.db, through SQLite's C API. The
/// connection opens with the first save, creating the table if missing, and every save runs the same prepared
/// INSERT OR REPLACE in autocommit, so each save is its own transaction, committed before it returns. No PRAGMA is set.
class SqliteMetadataAdapter : public ports::MetadataStorage {
public:
    explicit SqliteMetadataAdapter(std::filesystem::path database);

    SqliteMetadataAdapter(const SqliteMetadataAdapter&) = delete;
    SqliteMetadataAdapter& operator=(const SqliteMetadataAdapter&) = delete;

    ~SqliteMetadataAdapter() override;

    void save(const model::Book& book) override;

private:
    sqlite3_stmt* upsert();

    std::filesystem::path database_;
    sqlite3* connection_ = nullptr;
    sqlite3_stmt* upsert_ = nullptr;
};

}
