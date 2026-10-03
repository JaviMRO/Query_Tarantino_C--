#include "tarantino/crawler/adapters/datalake/BookFiles.hpp"

#include "tarantino/platform/Files.hpp"
#include "tarantino/platform/Paths.hpp"
#include "tarantino/platform/Text.hpp"

#include <algorithm>
#include <functional>
#include <limits>

namespace tarantino::crawler::adapters {

namespace files = platform::files;
using std::filesystem::path;

namespace {

constexpr std::string_view TEMPORARY_SUFFIX = ".tmp";
// Every layout names its files <prefix>header.txt and <prefix>body.txt (SPEC §6)
constexpr std::string_view HEADER_NAME = "header.txt";
constexpr std::string_view BODY_NAME = "body.txt";

void writeAtomically(const path& target, std::string_view content) {
    path temporary = target;
    temporary += TEMPORARY_SUFFIX;
    if (target.has_parent_path()) {
        files::createDirectories(target.parent_path());
    }
    files::writeString(temporary, content);
    files::move(temporary, target);
}

bool isIncomplete(const path& file) {
    const std::string name = platform::portable(file.filename());
    if (name.ends_with(TEMPORARY_SUFFIX)) {
        return true;
    }
    return name.ends_with(HEADER_NAME)
           && !files::exists(file.parent_path() / platform::pathOf(platform::replaceAll(name, HEADER_NAME, BODY_NAME)));
}

}

model::StoredPaths BookFiles::write(const model::StoredPaths& paths, const model::BookText& book) {
    writeAtomically(paths.header, book.header);
    writeAtomically(paths.body, book.body);
    return paths;
}

std::optional<model::StoredPaths> BookFiles::existing(model::StoredPaths paths) {
    if (files::exists(paths.body)) {
        return paths;
    }
    return std::nullopt;
}

std::vector<std::string> BookFiles::names(const path& directory) {
    return files::names(directory);
}

bool BookFiles::modifiedSince(const path& file, platform::Instant instant) {
    const std::optional<platform::Instant> modified = files::lastModifiedIfExists(file);
    return modified && !(*modified < instant);
}

std::optional<int> BookFiles::bookId(std::string_view name, std::string_view suffix) {
    if (!name.ends_with(suffix)) {
        return std::nullopt;
    }
    return platform::parseDigits(name.substr(0, name.size() - suffix.size()));
}

int BookFiles::removeIncompleteWrites(const path& root) {
    if (!files::isDirectory(root)) {
        return 0;
    }
    constexpr int wholeTree = std::numeric_limits<int>::max();
    std::vector<path> incomplete;
    files::walk(root, wholeTree, [&](const files::WalkedPath& walked) {
        if (walked.regularFile && isIncomplete(walked.path)) {
            incomplete.push_back(walked.path);
        }
        return true;
    });
    for (const path& file : incomplete) {
        files::remove(file);
    }
    std::vector<path> directories;
    files::walk(root, wholeTree, [&](const files::WalkedPath& walked) {
        if (walked.directory && walked.depth > 0) {
            directories.push_back(walked.path);
        }
        return true;
    });
    // In reverse order every directory comes after the ones inside it, which may leave it empty
    std::sort(directories.begin(), directories.end(), std::greater<>());
    for (const path& directory : directories) {
        if (files::isEmptyDirectory(directory)) {
            files::remove(directory);
        }
    }
    return static_cast<int>(incomplete.size());
}

path::string_type BookFiles::prefix(const path& directory) {
    return (directory / path()).native();
}

path::string_type BookFiles::text(std::string_view utf8) {
#ifdef _WIN32
    return platform::pathOf(utf8).native();
#else
    return path::string_type(utf8);
#endif
}

}
