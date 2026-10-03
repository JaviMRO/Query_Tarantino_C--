#include "tarantino/indexer/adapters/index/mongo/MongodbIndexAdapter.hpp"

#include "tarantino/indexer/adapters/mongo/MongoDatabases.hpp"

#include <bsoncxx/builder/basic/array.hpp>
#include <bsoncxx/builder/basic/document.hpp>
#include <bsoncxx/builder/basic/kvp.hpp>
#include <mongocxx/bulk_write.hpp>
#include <mongocxx/model/update_one.hpp>
#include <mongocxx/options/bulk_write.hpp>

namespace tarantino::indexer::adapters {

using bsoncxx::builder::basic::kvp;
using bsoncxx::builder::basic::make_document;

namespace {

constexpr const char* COLLECTION = "inverted_index";

}

MongodbIndexAdapter::MongodbIndexAdapter(const std::string& connectionUri)
    : collection_(MongoDatabases::database(connectionUri)[COLLECTION]) {
    collection_.create_index(make_document(kvp("term", 1)), make_document(kvp("unique", true)));
}

void MongodbIndexAdapter::add(const model::TermOccurrences& occurrences) {
    pending_.add(occurrences);
}

void MongodbIndexAdapter::flush() {
    const auto postings = pending_.drain();
    if (postings.empty()) {
        return;
    }
    mongocxx::options::bulk_write options;
    options.ordered(false);
    mongocxx::bulk_write updates = collection_.create_bulk_write(options);
    for (const auto& [term, ids] : postings) {
        bsoncxx::builder::basic::array newIds;
        for (const int bookId : ids) {
            newIds.append(bookId);
        }
        mongocxx::model::update_one update(
                make_document(kvp("term", term)),
                make_document(kvp("$addToSet", make_document(kvp("postings", make_document(kvp("$each", newIds.view())))))));
        update.upsert(true);
        updates.append(update);
    }
    updates.execute();
}

}
