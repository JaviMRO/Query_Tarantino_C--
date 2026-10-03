#include "tarantino/crawler/adapters/datalake/book/BookBasedDatalakeAdapter.hpp"

#include "tarantino/crawler/adapters/datalake/BookFiles.hpp"
#include "tarantino/platform/Paths.hpp"
#include "tarantino/platform/Text.hpp"

#include <string>

namespace tarantino::crawler::adapters {

using std::filesystem::path;

namespace {

constexpr const char* HEADER_FILE = "header.txt";
constexpr const char* BODY_FILE = "body.txt";

}

BookBasedDatalakeAdapter::BookBasedDatalakeAdapter(path root) : root_(std::move(root)), rootPrefix_(BookFiles::prefix(root_)) {
}

model::StoredPaths BookBasedDatalakeAdapter::save(const model::BookText& book) {
    return BookFiles::write(paths(book.bookId), book);
}

std::optional<model::StoredPaths> BookBasedDatalakeAdapter::pathsOf(int bookId) {
    return BookFiles::existing(paths(bookId));
}

std::unordered_set<int> BookBasedDatalakeAdapter::idsStoredSince(platform::Instant instant) {
    std::unordered_set<int> ids;
    const path::string_type body = BookFiles::text(BODY_FILE);
    for (const std::string& name : BookFiles::names(root_)) {
        const auto bookId = platform::parseDigits(name);
        if (!bookId) {
            continue;
        }
        path::string_type file = rootPrefix_;
        file += BookFiles::text(name);
        file += path::preferred_separator;
        file += body;
        if (BookFiles::modifiedSince(path(std::move(file)), instant)) {
            ids.insert(*bookId);
        }
    }
    return ids;
}

int BookBasedDatalakeAdapter::removeIncompleteWrites() {
    return BookFiles::removeIncompleteWrites(root_);
}

/// Built as text and made into paths once, which takes half the time of joining paths component by component: a
/// lookup is little more than this and the check of the body (lookup_time, SPEC §11).
model::StoredPaths BookBasedDatalakeAdapter::paths(int bookId) const {
    path::string_type directory = rootPrefix_;
    directory += BookFiles::text(std::to_string(bookId));
    directory += path::preferred_separator;
    return {path(directory + BookFiles::text(HEADER_FILE)), path(directory + BookFiles::text(BODY_FILE))};
}

}
