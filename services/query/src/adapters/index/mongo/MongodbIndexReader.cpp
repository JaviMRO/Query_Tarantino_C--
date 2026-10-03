#include "tarantino/query/adapters/index/mongo/MongodbIndexReader.hpp"

#include "tarantino/query/adapters/mongo/MongoDatabases.hpp"

#include <bsoncxx/builder/basic/document.hpp>
#include <bsoncxx/builder/basic/kvp.hpp>

namespace tarantino::query::adapters {

using bsoncxx::builder::basic::kvp;
using bsoncxx::builder::basic::make_document;

namespace {

constexpr const char* COLLECTION = "inverted_index";

int bookId(const bsoncxx::array::element& element) {
    return element.type() == bsoncxx::type::k_int64 ? static_cast<int>(element.get_int64().value) : element.get_int32().value;
}

}

MongodbIndexReader::MongodbIndexReader(const std::string& connectionUri)
    : collection_(MongoDatabases::database(connectionUri)[COLLECTION]) {
}

ports::InvertedIndexReader::Postings MongodbIndexReader::postings(const std::string& term) {
    const auto document = collection_.find_one(make_document(kvp("term", term)));
    if (!document) {
        return Postings::none();
    }
    std::unordered_set<int> ids;
    for (const bsoncxx::array::element& element : document->view()["postings"].get_array().value) {
        ids.insert(bookId(element));
    }
    return Postings::owning(std::move(ids));
}

}
