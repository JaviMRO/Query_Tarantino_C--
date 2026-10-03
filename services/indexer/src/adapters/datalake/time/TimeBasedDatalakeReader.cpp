#include "tarantino/indexer/adapters/datalake/time/TimeBasedDatalakeReader.hpp"

#include "tarantino/indexer/adapters/datalake/BookFiles.hpp"
#include "tarantino/platform/Files.hpp"

#include <string>

namespace tarantino::indexer::adapters {

namespace {

constexpr const char* HEADER_SUFFIX = ".header.txt";
constexpr const char* BODY_SUFFIX = ".body.txt";
// Day, hour and book file below the root
constexpr int BOOK_FILE_DEPTH = 3;

}

TimeBasedDatalakeReader::TimeBasedDatalakeReader(std::filesystem::path root) : root_(std::move(root)) {
}

std::optional<model::BookText> TimeBasedDatalakeReader::bookText(int bookId) {
    if (!platform::files::isDirectory(root_)) {
        return std::nullopt;
    }
    const std::string id = std::to_string(bookId);
    const std::filesystem::path name = id + BODY_SUFFIX;
    const auto body = platform::files::find(root_, BOOK_FILE_DEPTH,
                                            [&](const std::filesystem::path& candidate) { return candidate.filename() == name; });
    if (!body) {
        return std::nullopt;
    }
    return BookFiles::bookText(bookId, body->parent_path() / (id + HEADER_SUFFIX), *body);
}

}
