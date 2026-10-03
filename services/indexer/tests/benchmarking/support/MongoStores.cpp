#include "indexer/tests/benchmarking/support/MongoStores.hpp"

#include "tarantino/platform/MongoDriver.hpp"

#include <bsoncxx/builder/basic/array.hpp>
#include <bsoncxx/builder/basic/document.hpp>
#include <bsoncxx/builder/basic/kvp.hpp>
#include <mongocxx/client.hpp>
#include <mongocxx/options/index.hpp>
#include <mongocxx/options/insert.hpp>
#include <mongocxx/pipeline.hpp>
#include <mongocxx/uri.hpp>

#include <exception>
#include <string>
#include <vector>

namespace tarantino::indexer::benchmarking {

using bsoncxx::builder::basic::kvp;
using bsoncxx::builder::basic::make_document;

namespace {

constexpr const char* DEFAULT_INDEX = "_id_";

struct Connection {
    explicit Connection(const std::string& connectionUri) : uri([&] {
        platform::startMongoDriver();
        return mongocxx::uri{connectionUri};
    }()), client(uri) {
    }

    mongocxx::database database() {
        return client[uri.database()];
    }

    mongocxx::uri uri;
    mongocxx::client client;
};

std::int64_t number(const bsoncxx::document::element& element) {
    switch (element.type()) {
        case bsoncxx::type::k_int32:
            return element.get_int32().value;
        case bsoncxx::type::k_int64:
            return element.get_int64().value;
        case bsoncxx::type::k_double:
            return static_cast<std::int64_t>(element.get_double().value);
        default:
            return 0;
    }
}

}

void MongoStores::drop(const std::string& connectionUri) {
    Connection(connectionUri).database().drop();
}

void MongoStores::copy(const std::string& sourceUri, const std::string& targetUri) {
    Connection connection(sourceUri);
    mongocxx::database source = connection.database();
    const std::string target = mongocxx::uri{targetUri}.database();
    for (const std::string& name : source.list_collection_names()) {
        mongocxx::collection collection = source[name];
        mongocxx::pipeline pipeline;
        pipeline.append_stage(make_document(kvp("$out", make_document(kvp("db", target), kvp("coll", name)))));
        for (auto&& ignored : collection.aggregate(pipeline)) {
            (void)ignored;
        }
        for (const bsoncxx::document::view& index : collection.list_indexes()) {
            const std::string indexName(index["name"].get_string().value);
            if (indexName == DEFAULT_INDEX) {
                continue;
            }
            const bool unique = index["unique"] && index["unique"].get_bool().value;
            connection.client[target][name].create_index(bsoncxx::document::value(index["key"].get_document().value),
                                                         make_document(kvp("name", indexName), kvp("unique", unique)));
        }
    }
}

void MongoStores::restoreDocuments(const std::string& snapshotUri, const std::string& targetUri, const std::string& collection,
                                   const std::string& field, const std::unordered_set<std::string>& values) {
    Connection connection(snapshotUri);
    bsoncxx::builder::basic::array inValues;
    for (const std::string& value : values) {
        inValues.append(value);
    }
    const auto documentsOfValues = make_document(kvp(field, make_document(kvp("$in", inValues.view()))));
    mongocxx::collection target = connection.client[mongocxx::uri{targetUri}.database()][collection];
    target.delete_many(documentsOfValues.view());
    std::vector<bsoncxx::document::value> stored;
    for (const bsoncxx::document::view& document : connection.database()[collection].find(documentsOfValues.view())) {
        stored.emplace_back(document);
    }
    if (!stored.empty()) {
        mongocxx::options::insert options;
        options.ordered(false);
        target.insert_many(stored, options);
    }
}

std::uint64_t MongoStores::documentCount(const std::string& connectionUri, const std::string& collection) {
    return static_cast<std::uint64_t>(Connection(connectionUri).database()[collection].count_documents({}));
}

std::uint64_t MongoStores::diskUsage(const std::string& connectionUri) {
    Connection connection(connectionUri);
    connection.client["admin"].run_command(make_document(kvp("fsync", 1)));
    mongocxx::database database = connection.database();
    std::uint64_t bytes = 0;
    for (const std::string& name : database.list_collection_names()) {
        mongocxx::pipeline statistics;
        statistics.append_stage(make_document(kvp("$collStats", make_document(kvp("storageStats", make_document())))));
        for (const bsoncxx::document::view& result : database[name].aggregate(statistics)) {
            const bsoncxx::document::view storage = result["storageStats"].get_document().value;
            bytes += static_cast<std::uint64_t>(number(storage["storageSize"]) + number(storage["totalIndexSize"]));
        }
    }
    return bytes;
}

std::string MongoStores::serverVersion(const std::string& connectionUri) {
    try {
        Connection connection(connectionUri);
        const bsoncxx::document::value info = connection.client["admin"].run_command(make_document(kvp("buildInfo", 1)));
        return std::string(info.view()["version"].get_string().value);
    } catch (const std::exception& error) {
        return std::string("unknown, the server did not answer: ") + error.what();
    }
}

}
