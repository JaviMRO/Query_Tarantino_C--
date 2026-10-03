#include "tarantino/control/adapters/FileControlStateStore.hpp"

#include "tarantino/platform/Files.hpp"
#include "tarantino/platform/Text.hpp"

#include <string>
#include <unordered_set>

namespace tarantino::control::adapters {

namespace files = platform::files;

namespace {

/// The distinct ids of the file, in file order. Read as bytes: only lines of digits count anyway.
std::vector<int> ids(const std::filesystem::path& file) {
    std::vector<int> ids;
    if (!files::exists(file)) {
        return ids;
    }
    const std::string content = files::readBytes(file);
    std::unordered_set<int> seen;
    for (const std::string_view line : platform::lines(content)) {
        const auto bookId = platform::parseDigits(platform::strip(line));
        if (bookId && seen.insert(*bookId).second) {
            ids.push_back(*bookId);
        }
    }
    return ids;
}

void append(const std::filesystem::path& file, int bookId) {
    files::createDirectories(std::filesystem::absolute(file).parent_path());
    files::append(file, std::to_string(bookId) + "\n");
}

}

FileControlStateStore::FileControlStateStore(const std::filesystem::path& root)
    : downloaded_(root / "downloaded_books.txt"), indexed_(root / "indexed_books.txt") {
}

std::vector<int> FileControlStateStore::downloaded() {
    return ids(downloaded_);
}

std::vector<int> FileControlStateStore::indexed() {
    return ids(indexed_);
}

void FileControlStateStore::markDownloaded(int bookId) {
    append(downloaded_, bookId);
}

void FileControlStateStore::markIndexed(int bookId) {
    append(indexed_, bookId);
}

}
