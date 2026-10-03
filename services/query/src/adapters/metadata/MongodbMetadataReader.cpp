#include "tarantino/query/adapters/metadata/MongodbMetadataReader.hpp"

#include "tarantino/query/adapters/mongo/MongoDatabases.hpp"

#include <bsoncxx/builder/basic/document.hpp>
#include <bsoncxx/builder/basic/kvp.hpp>
#include <mongocxx/options/find.hpp>

#include <string_view>

namespace tarantino::query::adapters {

using bsoncxx::builder::basic::kvp;
using bsoncxx::builder::basic::make_document;

namespace {

constexpr const char* COLLECTION = "books";
constexpr std::string_view METACHARACTERS = "\\^$.|?*+()[]{}";

std::optional<std::string> text(const bsoncxx::document::view& document, const char* field) {
    const auto element = document[field];
    if (!element || element.type() != bsoncxx::type::k_string) {
        return std::nullopt;
    }
    return std::string(element.get_string().value);
}

int bookIdOf(const bsoncxx::document::view& document) {
    const auto element = document["book_id"];
    return element.type() == bsoncxx::type::k_int64 ? static_cast<int>(element.get_int64().value) : element.get_int32().value;
}

model::BookMetadata bookOf(const bsoncxx::document::view& document) {
    return model::BookMetadata{bookIdOf(document), text(document, "title"), text(document, "author"),
                               text(document, "language"), text(document, "path").value_or("")};
}

}

MongodbMetadataReader::MongodbMetadataReader(const std::string& connectionUri)
    : collection_(MongoDatabases::database(connectionUri)[COLLECTION]) {
}

std::optional<model::BookMetadata> MongodbMetadataReader::book(int bookId) {
    const auto document = collection_.find_one(make_document(kvp("book_id", bookId)));
    if (!document) {
        return std::nullopt;
    }
    return bookOf(document->view());
}

std::vector<model::BookMetadata> MongodbMetadataReader::booksBy(const std::string& author) {
    mongocxx::options::find options;
    options.sort(make_document(kvp("book_id", 1)));
    std::vector<model::BookMetadata> books;
    for (const bsoncxx::document::view& document :
         collection_.find(make_document(kvp("author", make_document(kvp("$regex", quoted(author)), kvp("$options", "i")))), options)) {
        books.push_back(bookOf(document));
    }
    return books;
}

std::string MongodbMetadataReader::quoted(const std::string& text) {
    std::string pattern;
    pattern.reserve(text.size() + 8);
    for (const char character : text) {
        if (METACHARACTERS.find(character) != std::string_view::npos) {
            pattern.push_back('\\');
        }
        pattern.push_back(character);
    }
    return pattern;
}

}
