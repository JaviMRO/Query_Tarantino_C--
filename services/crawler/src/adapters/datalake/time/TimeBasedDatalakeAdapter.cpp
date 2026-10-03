#include "tarantino/crawler/adapters/datalake/time/TimeBasedDatalakeAdapter.hpp"

#include "tarantino/crawler/adapters/datalake/BookFiles.hpp"
#include "tarantino/platform/Files.hpp"
#include "tarantino/platform/Paths.hpp"

#include <string>

namespace tarantino::crawler::adapters {

using std::filesystem::path;

namespace {

constexpr std::string_view HEADER_SUFFIX = ".header.txt";
constexpr std::string_view BODY_SUFFIX = ".body.txt";
// Day, hour and book file below the root
constexpr int BOOK_FILE_DEPTH = 3;

model::StoredPaths pathsIn(const path& directory, int bookId) {
    const std::string id = std::to_string(bookId);
    return {directory / (id + std::string(HEADER_SUFFIX)), directory / (id + std::string(BODY_SUFFIX))};
}

}

TimeBasedDatalakeAdapter::TimeBasedDatalakeAdapter(path root, platform::Clock clock)
    : root_(std::move(root)), clock_(std::move(clock)) {
}

model::StoredPaths TimeBasedDatalakeAdapter::save(const model::BookText& book) {
    return BookFiles::write(pathsIn(currentDirectory(), book.bookId), book);
}

std::optional<model::StoredPaths> TimeBasedDatalakeAdapter::pathsOf(int bookId) {
    const auto body = bodyFile(bookId);
    if (!body) {
        return std::nullopt;
    }
    return pathsIn(body->parent_path(), bookId);
}

std::unordered_set<int> TimeBasedDatalakeAdapter::idsStoredSince(platform::Instant instant) {
    // Hour directories from the hour that holds the instant on, by their names alone (SPEC §6)
    const std::string firstHour = platform::utcDay(instant) + platform::utcHour(instant);
    std::unordered_set<int> ids;
    const path::string_type rootPrefix = BookFiles::prefix(root_);
    for (const std::string& day : BookFiles::names(root_)) {
        path::string_type dayPrefix = rootPrefix + BookFiles::text(day);
        const path dayDirectory(dayPrefix);
        dayPrefix += path::preferred_separator;
        for (const std::string& hour : BookFiles::names(dayDirectory)) {
            if (day + hour < firstHour) {
                continue;
            }
            for (const std::string& name : BookFiles::names(path(dayPrefix + BookFiles::text(hour)))) {
                if (const auto bookId = BookFiles::bookId(name, BODY_SUFFIX)) {
                    ids.insert(*bookId);
                }
            }
        }
    }
    return ids;
}

int TimeBasedDatalakeAdapter::removeIncompleteWrites() {
    return BookFiles::removeIncompleteWrites(root_);
}

path TimeBasedDatalakeAdapter::currentDirectory() const {
    const platform::Instant now = clock_();
    return root_ / platform::utcDay(now) / platform::utcHour(now);
}

std::optional<path> TimeBasedDatalakeAdapter::bodyFile(int bookId) const {
    if (!platform::files::isDirectory(root_)) {
        return std::nullopt;
    }
    // Depth first, up to the depth of book files, stopping at the first one with the body's name (SPEC §6)
    const path name = std::to_string(bookId) + std::string(BODY_SUFFIX);
    return platform::files::find(root_, BOOK_FILE_DEPTH, [&](const path& candidate) { return candidate.filename() == name; });
}

}
