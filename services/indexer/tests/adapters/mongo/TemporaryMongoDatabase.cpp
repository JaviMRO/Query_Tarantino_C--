#include "indexer/tests/adapters/mongo/TemporaryMongoDatabase.hpp"

#include "tarantino/platform/Environment.hpp"
#include "tarantino/platform/MongoDriver.hpp"

#include <bsoncxx/builder/basic/document.hpp>
#include <bsoncxx/builder/basic/kvp.hpp>
#include <mongocxx/client.hpp>
#include <mongocxx/uri.hpp>

#include <chrono>
#include <random>

namespace tarantino::indexer::adapters {

namespace {

using bsoncxx::builder::basic::kvp;
using bsoncxx::builder::basic::make_document;

std::string configuredServer() {
    return platform::environmentVariable("TARANTINO_MONGO_URI", "mongodb://localhost:27017");
}

/// Looked for once per process, by the first test that needs MongoDB: a ping that waits at most 2 seconds.
bool serverAnswers() {
    static const bool answers = [] {
        try {
            platform::startMongoDriver();
            const std::string server = configuredServer();
            const char separator = server.find('?') == std::string::npos ? '?' : '&';
            std::string uri = server;
            if (separator == '?' && uri.find('/', uri.find("://") + 3) == std::string::npos) {
                uri += '/';
            }
            uri += separator;
            uri += "serverSelectionTimeoutMS=2000";
            mongocxx::client client{mongocxx::uri{uri}};
            client["admin"].run_command(make_document(kvp("ping", 1)));
            return true;
        } catch (const std::exception&) {
            return false;
        }
    }();
    return answers;
}

}

TemporaryMongoDatabase::TemporaryMongoDatabase() {
    std::mt19937_64 random(std::random_device{}() ^ static_cast<std::uint64_t>(std::chrono::steady_clock::now().time_since_epoch().count()));
    name_ = "tarantino_test_" + std::to_string(random());
}

TemporaryMongoDatabase::~TemporaryMongoDatabase() {
    if (!uri_) {
        return;
    }
    try {
        mongocxx::client client{mongocxx::uri{*uri_}};
        client[name_].drop();
    } catch (const std::exception&) {
        // The server went away: nothing left to drop
    }
}

std::optional<std::string> TemporaryMongoDatabase::uri() {
    if (!serverAnswers()) {
        return std::nullopt;
    }
    if (!uri_) {
        uri_ = withDatabase(configuredServer(), name_);
    }
    return uri_;
}

std::string TemporaryMongoDatabase::unavailable() {
    return "No MongoDB answers at " + configuredServer();
}

std::string TemporaryMongoDatabase::withDatabase(const std::string& connectionUri, const std::string& database) {
    const std::size_t hosts = connectionUri.find("://");
    const std::size_t path = connectionUri.find('/', hosts == std::string::npos ? 0 : hosts + 3);
    if (path == std::string::npos) {
        const std::size_t options = connectionUri.find('?');
        if (options == std::string::npos) {
            return connectionUri + "/" + database;
        }
        return connectionUri.substr(0, options) + "/" + database + connectionUri.substr(options);
    }
    const std::size_t options = connectionUri.find('?', path);
    return connectionUri.substr(0, path) + "/" + database + (options == std::string::npos ? "" : connectionUri.substr(options));
}

}
