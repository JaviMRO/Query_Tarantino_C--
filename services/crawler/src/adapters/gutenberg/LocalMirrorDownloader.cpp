#include "tarantino/crawler/adapters/gutenberg/LocalMirrorDownloader.hpp"

#include "tarantino/crawler/model/failure/DownloadException.hpp"
#include "tarantino/platform/Files.hpp"
#include "tarantino/platform/Paths.hpp"
#include "tarantino/platform/Text.hpp"

#include <algorithm>
#include <string>

namespace tarantino::crawler::adapters {

namespace files = platform::files;
using model::DownloadException;
using model::FailureReason;

namespace {

/// A directory name that is a book id: [1-9][0-9]{0,8}.
bool isBookId(std::string_view name) {
    return !name.empty() && name.size() <= 9 && name.front() != '0'
           && std::all_of(name.begin(), name.end(), [](char c) { return c >= '0' && c <= '9'; });
}

}

LocalMirrorDownloader::LocalMirrorDownloader(std::filesystem::path mirror) : mirror_(std::move(mirror)) {
}

std::string LocalMirrorDownloader::rawText(int bookId) {
    const std::filesystem::path text = file(bookId);
    if (!files::isRegularFile(text)) {
        throw DownloadException(FailureReason::NotFound, "Book " + std::to_string(bookId) + " not found in the mirror "
                                                                  + platform::portable(mirror_));
    }
    try {
        return files::readString(text);
    } catch (const std::filesystem::filesystem_error& error) {
        throw DownloadException(FailureReason::NetworkError,
                                "Could not read book " + std::to_string(bookId) + " from the mirror: " + error.what());
    }
}

std::vector<int> LocalMirrorDownloader::bookIds() const {
    std::vector<int> ids;
    for (const std::filesystem::path& entry : files::list(mirror_)) {
        const std::string name = platform::portable(entry.filename());
        if (!isBookId(name)) {
            continue;
        }
        const int bookId = *platform::parseDigits(name);
        if (files::isRegularFile(file(bookId))) {
            ids.push_back(bookId);
        }
    }
    std::sort(ids.begin(), ids.end());
    return ids;
}

std::filesystem::path LocalMirrorDownloader::file(int bookId) const {
    const std::string id = std::to_string(bookId);
    return mirror_ / id / ("pg" + id + ".txt");
}

}
