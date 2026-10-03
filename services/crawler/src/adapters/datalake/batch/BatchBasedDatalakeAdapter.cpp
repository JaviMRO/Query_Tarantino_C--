#include "tarantino/crawler/adapters/datalake/batch/BatchBasedDatalakeAdapter.hpp"

#include "tarantino/crawler/adapters/datalake/BookFiles.hpp"
#include "tarantino/platform/Paths.hpp"
#include "tarantino/platform/Text.hpp"

#include <string>

namespace tarantino::crawler::adapters {

using std::filesystem::path;

namespace {

constexpr int BATCH_SIZE = 1000;
constexpr std::string_view HEADER_SUFFIX = ".header.txt";
constexpr std::string_view BODY_SUFFIX = ".body.txt";

}

BatchBasedDatalakeAdapter::BatchBasedDatalakeAdapter(path root) : root_(std::move(root)), rootPrefix_(BookFiles::prefix(root_)) {
}

model::StoredPaths BatchBasedDatalakeAdapter::save(const model::BookText& book) {
    return BookFiles::write(paths(book.bookId), book);
}

std::optional<model::StoredPaths> BatchBasedDatalakeAdapter::pathsOf(int bookId) {
    return BookFiles::existing(paths(bookId));
}

std::unordered_set<int> BatchBasedDatalakeAdapter::idsStoredSince(platform::Instant instant) {
    std::unordered_set<int> ids;
    for (const std::string& batch : BookFiles::names(root_)) {
        path::string_type batchPrefix = rootPrefix_ + BookFiles::text(batch);
        const path batchDirectory(batchPrefix);
        batchPrefix += path::preferred_separator;
        for (const std::string& name : BookFiles::names(batchDirectory)) {
            const auto bookId = BookFiles::bookId(name, BODY_SUFFIX);
            if (bookId && BookFiles::modifiedSince(path(batchPrefix + BookFiles::text(name)), instant)) {
                ids.insert(*bookId);
            }
        }
    }
    return ids;
}

int BatchBasedDatalakeAdapter::removeIncompleteWrites() {
    return BookFiles::removeIncompleteWrites(root_);
}

/// Built as text and made into paths once, as the book layout does.
model::StoredPaths BatchBasedDatalakeAdapter::paths(int bookId) const {
    path::string_type file = rootPrefix_;
    file += BookFiles::text(std::to_string(bookId / BATCH_SIZE));
    file += path::preferred_separator;
    file += BookFiles::text(std::to_string(bookId));
    return {path(file + BookFiles::text(HEADER_SUFFIX)), path(file + BookFiles::text(BODY_SUFFIX))};
}

}
