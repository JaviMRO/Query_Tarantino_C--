#include "tarantino/indexer/adapters/metadata/MongodbMetadataAdapter.hpp"

#include "adapters/metadata/PortablePaths.hpp"
#include "tarantino/indexer/adapters/mongo/MongoDatabases.hpp"

#include <bsoncxx/builder/basic/document.hpp>
#include <bsoncxx/builder/basic/kvp.hpp>
#include <bsoncxx/types.hpp>
#include <mongocxx/options/replace.hpp>

#include <string_view>

namespace tarantino::indexer::adapters {

using bsoncxx::builder::basic::kvp;
using bsoncxx::builder::basic::make_document;

namespace {

constexpr const char* COLLECTION = "books";

/// A missing field is stored as null (SPEC §8.2).
void appendText(bsoncxx::builder::basic::document& document, std::string_view field, const std::optional<std::string>& value) {
    if (value) {
        document.append(kvp(field, *value));
    } else {
        document.append(kvp(field, bsoncxx::types::b_null{}));
    }
}

}

MongodbMetadataAdapter::MongodbMetadataAdapter(const std::string& connectionUri)
    : collection_(MongoDatabases::database(connectionUri)[COLLECTION]) {
    collection_.create_index(make_document(kvp("book_id", 1)), make_document(kvp("unique", true)));
}

void MongodbMetadataAdapter::save(const model::Book& book) {
    bsoncxx::builder::basic::document document;
    document.append(kvp("book_id", book.bookId));
    appendText(document, "title", book.title);
    appendText(document, "author", book.author);
    appendText(document, "language", book.language);
    document.append(kvp("path", PortablePaths::of(book.path)));
    mongocxx::options::replace options;
    options.upsert(true);
    collection_.replace_one(make_document(kvp("book_id", book.bookId)), document.view(), options);
}

}
