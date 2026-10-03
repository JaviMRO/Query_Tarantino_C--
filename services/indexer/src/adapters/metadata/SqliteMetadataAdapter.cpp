#include "tarantino/indexer/adapters/metadata/SqliteMetadataAdapter.hpp"

#include "adapters/metadata/PortablePaths.hpp"
#include "tarantino/platform/Files.hpp"

#include <sqlite3.h>

#include <stdexcept>
#include <string>

namespace tarantino::indexer::adapters {

namespace {

constexpr const char* CREATE_TABLE = R"(CREATE TABLE IF NOT EXISTS books (
    book_id  INTEGER PRIMARY KEY,
    title    TEXT,
    author   TEXT,
    language TEXT,
    path     TEXT NOT NULL
))";
constexpr const char* UPSERT = "INSERT OR REPLACE INTO books (book_id, title, author, language, path) VALUES (?, ?, ?, ?, ?)";
// What Java's sqlite-jdbc waits by default for a database another process is writing
constexpr int BUSY_TIMEOUT_MILLISECONDS = 3000;

void bindText(sqlite3_stmt* statement, int index, const std::optional<std::string>& value) {
    if (value) {
        sqlite3_bind_text(statement, index, value->data(), static_cast<int>(value->size()), SQLITE_TRANSIENT);
    } else {
        sqlite3_bind_null(statement, index);
    }
}

}

SqliteMetadataAdapter::SqliteMetadataAdapter(std::filesystem::path database) : database_(std::move(database)) {
}

SqliteMetadataAdapter::~SqliteMetadataAdapter() {
    sqlite3_finalize(upsert_);
    sqlite3_close(connection_);
}

void SqliteMetadataAdapter::save(const model::Book& book) {
    sqlite3_stmt* statement = upsert();
    sqlite3_bind_int(statement, 1, book.bookId);
    bindText(statement, 2, book.title);
    bindText(statement, 3, book.author);
    bindText(statement, 4, book.language);
    const std::string path = PortablePaths::of(book.path);
    sqlite3_bind_text(statement, 5, path.data(), static_cast<int>(path.size()), SQLITE_TRANSIENT);
    const int result = sqlite3_step(statement);
    sqlite3_reset(statement);
    if (result != SQLITE_DONE) {
        throw std::runtime_error("Could not save metadata of book " + std::to_string(book.bookId) + ": "
                                 + sqlite3_errmsg(connection_));
    }
}

sqlite3_stmt* SqliteMetadataAdapter::upsert() {
    if (upsert_ != nullptr) {
        return upsert_;
    }
    platform::files::createDirectories(std::filesystem::absolute(database_).parent_path());
    const std::string file = platform::portable(database_);
    if (sqlite3_open_v2(file.c_str(), &connection_, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, nullptr) != SQLITE_OK
        || sqlite3_busy_timeout(connection_, BUSY_TIMEOUT_MILLISECONDS) != SQLITE_OK
        || sqlite3_exec(connection_, CREATE_TABLE, nullptr, nullptr, nullptr) != SQLITE_OK
        || sqlite3_prepare_v2(connection_, UPSERT, -1, &upsert_, nullptr) != SQLITE_OK) {
        const std::string error = connection_ != nullptr ? sqlite3_errmsg(connection_) : "out of memory";
        sqlite3_finalize(upsert_);
        upsert_ = nullptr;
        sqlite3_close(connection_);
        connection_ = nullptr;
        throw std::runtime_error("Could not open " + file + ": " + error);
    }
    return upsert_;
}

}
