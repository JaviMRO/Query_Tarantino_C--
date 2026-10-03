#include "tarantino/query/adapters/metadata/SqliteMetadataReader.hpp"

#include "tarantino/platform/Files.hpp"
#include "tarantino/platform/Paths.hpp"

#include <sqlite3.h>

#include <stdexcept>

namespace tarantino::query::adapters {

namespace {

constexpr const char* BOOK_BY_ID = "SELECT book_id, title, author, language, path FROM books WHERE book_id = ?";
constexpr const char* BOOKS_BY_AUTHOR =
        "SELECT book_id, title, author, language, path FROM books WHERE author LIKE ? ESCAPE '\\' ORDER BY book_id";
// What Java's sqlite-jdbc waits by default for a database another process is writing
constexpr int BUSY_TIMEOUT_MILLISECONDS = 3000;

/// The author as a LIKE pattern that matches it literally: % and _ are not wildcards (SPEC §8.2).
std::string literal(const std::string& author) {
    std::string pattern;
    pattern.reserve(author.size() + 2);
    for (const char character : author) {
        if (character == '\\' || character == '%' || character == '_') {
            pattern.push_back('\\');
        }
        pattern.push_back(character);
    }
    return pattern;
}

std::optional<std::string> text(sqlite3_stmt* row, int column) {
    const unsigned char* value = sqlite3_column_text(row, column);
    if (value == nullptr) {
        return std::nullopt;
    }
    return std::string(reinterpret_cast<const char*>(value), static_cast<std::size_t>(sqlite3_column_bytes(row, column)));
}

}

SqliteMetadataReader::SqliteMetadataReader(std::filesystem::path database) : database_(std::move(database)) {
}

SqliteMetadataReader::~SqliteMetadataReader() {
    for (const auto& [sql, prepared] : statements_) {
        sqlite3_finalize(prepared);
    }
    sqlite3_close(connection_);
}

std::optional<model::BookMetadata> SqliteMetadataReader::book(int bookId) {
    if (!available()) {
        return std::nullopt;
    }
    sqlite3_stmt* query = statement(BOOK_BY_ID);
    sqlite3_bind_int(query, 1, bookId);
    std::vector<model::BookMetadata> found = books(query);
    if (found.empty()) {
        return std::nullopt;
    }
    return std::move(found.front());
}

std::vector<model::BookMetadata> SqliteMetadataReader::booksBy(const std::string& author) {
    if (!available()) {
        return {};
    }
    sqlite3_stmt* query = statement(BOOKS_BY_AUTHOR);
    const std::string pattern = "%" + literal(author) + "%";
    sqlite3_bind_text(query, 1, pattern.data(), static_cast<int>(pattern.size()), SQLITE_TRANSIENT);
    return books(query);
}

bool SqliteMetadataReader::available() {
    return connection_ != nullptr || platform::files::exists(database_);
}

sqlite3_stmt* SqliteMetadataReader::statement(const char* sql) {
    if (connection_ == nullptr) {
        const std::string file = platform::portable(database_);
        if (sqlite3_open_v2(file.c_str(), &connection_, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, nullptr) != SQLITE_OK) {
            const std::string error = connection_ != nullptr ? sqlite3_errmsg(connection_) : "out of memory";
            sqlite3_close(connection_);
            connection_ = nullptr;
            throw std::runtime_error("Could not query " + file + ": " + error);
        }
        sqlite3_busy_timeout(connection_, BUSY_TIMEOUT_MILLISECONDS);
    }
    sqlite3_stmt*& prepared = statements_[sql];
    if (prepared == nullptr && sqlite3_prepare_v2(connection_, sql, -1, &prepared, nullptr) != SQLITE_OK) {
        statements_.erase(sql);
        throw std::runtime_error("Could not query " + platform::portable(database_) + ": " + sqlite3_errmsg(connection_));
    }
    return prepared;
}

std::vector<model::BookMetadata> SqliteMetadataReader::books(sqlite3_stmt* query) {
    std::vector<model::BookMetadata> found;
    int result = SQLITE_ROW;
    while ((result = sqlite3_step(query)) == SQLITE_ROW) {
        found.push_back(model::BookMetadata{sqlite3_column_int(query, 0), text(query, 1), text(query, 2), text(query, 3),
                                            text(query, 4).value_or("")});
    }
    // Reset at once, so that no read transaction stays open between queries
    sqlite3_reset(query);
    sqlite3_clear_bindings(query);
    if (result != SQLITE_DONE) {
        throw std::runtime_error("Could not query " + platform::portable(database_) + ": " + sqlite3_errmsg(connection_));
    }
    return found;
}

}
