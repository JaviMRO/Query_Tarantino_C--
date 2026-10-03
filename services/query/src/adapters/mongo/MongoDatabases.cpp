#include "tarantino/query/adapters/mongo/MongoDatabases.hpp"

#include "tarantino/platform/MongoDriver.hpp"

#include <mongocxx/client.hpp>
#include <mongocxx/uri.hpp>

#include <map>
#include <memory>
#include <mutex>

namespace tarantino::query::adapters {

namespace {

constexpr const char* DEFAULT_DATABASE = "tarantino";

}

mongocxx::database MongoDatabases::database(const std::string& connectionUri) {
    platform::startMongoDriver();
    // Constructed after the driver's instance, so destroyed before it
    static std::mutex mutex;
    static std::map<std::string, std::unique_ptr<mongocxx::client>> clients;
    const std::lock_guard lock(mutex);
    std::unique_ptr<mongocxx::client>& client = clients[connectionUri];
    if (!client) {
        client = std::make_unique<mongocxx::client>(mongocxx::uri{connectionUri});
    }
    return (*client)[databaseName(connectionUri)];
}

std::string MongoDatabases::databaseName(const std::string& connectionUri) {
    platform::startMongoDriver();
    const std::string name = mongocxx::uri{connectionUri}.database();
    return name.empty() ? DEFAULT_DATABASE : name;
}

}
