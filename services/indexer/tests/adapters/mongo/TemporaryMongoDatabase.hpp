#pragma once

#include <optional>
#include <string>

namespace tarantino::indexer::adapters {

/// A database of its own for the tests that need MongoDB, dropped when the test ends. It lives on the server of
/// TARANTINO_MONGO_URI (localhost:27017 by default) when it answers, such as a native install; otherwise the tests
/// that ask for it are skipped, never failed (SPEC §17: C++ has no Testcontainers).
class TemporaryMongoDatabase {
public:
    TemporaryMongoDatabase();

    TemporaryMongoDatabase(const TemporaryMongoDatabase&) = delete;
    TemporaryMongoDatabase& operator=(const TemporaryMongoDatabase&) = delete;

    ~TemporaryMongoDatabase();

    /// The URI of the database, or none when no MongoDB answers: the test then calls GTEST_SKIP() << unavailable().
    std::optional<std::string> uri();

    static std::string unavailable();

    /// The same connection string with another database: hosts, credentials and options are kept.
    static std::string withDatabase(const std::string& connectionUri, const std::string& database);

private:
    std::string name_;
    std::optional<std::string> uri_;
};

}
