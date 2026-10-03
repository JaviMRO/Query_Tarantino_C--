#pragma once

#include <mongocxx/database.hpp>

#include <string>

namespace tarantino::query::adapters {

/// The databases of MongoDB (SPEC §8.1): each connection string has one client per process, opened the first time it
/// is needed and reused for every database and collection, with the driver's default pool, timeouts and write concern.
class MongoDatabases {
public:
    MongoDatabases() = delete;

    /// The database named in the path of the connection string, or tarantino when it names none.
    static mongocxx::database database(const std::string& connectionUri);

    static std::string databaseName(const std::string& connectionUri);
};

}
