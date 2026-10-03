#include "tarantino/indexer/adapters/index/folders/FolderPerTermIndexAdapter.hpp"

#include "adapters/index/folders/TermFiles.hpp"
#include "tarantino/platform/Files.hpp"
#include "tarantino/platform/Paths.hpp"
#include "tarantino/platform/Text.hpp"

#include <charconv>
#include <stdexcept>
#include <unordered_set>

namespace tarantino::indexer::adapters {

namespace files = platform::files;
using std::filesystem::path;

namespace {

std::set<int> storedPostings(const path& file) {
    std::set<int> postings;
    if (!files::exists(file)) {
        return postings;
    }
    const std::string content = files::readString(file);
    for (const std::string_view line : platform::lines(content)) {
        const auto bookId = platform::parseDigits(line);
        if (!bookId) {
            throw std::runtime_error("Not a book id in " + platform::portable(file) + ": " + std::string(line));
        }
        postings.insert(*bookId);
    }
    return postings;
}

std::string content(const std::set<int>& postings) {
    std::string text;
    text.reserve(postings.size() * 6);
    char digits[16];
    for (const int bookId : postings) {
        const auto [end, error] = std::to_chars(digits, digits + sizeof digits, bookId);
        text.append(digits, end);
        text.push_back('\n');
    }
    return text;
}

void writeAtomically(const path& file, const std::set<int>& postings) {
    path temporary = file;
    temporary += ".tmp";
    files::writeString(temporary, content(postings));
    files::move(temporary, file);
}

/// Writes the term file with the new ids only if it gains one, creating its folder the first time.
void merge(const path& file, const std::set<int>& ids, std::unordered_set<path::string_type>& folders) {
    std::set<int> postings = storedPostings(file);
    const std::size_t stored = postings.size();
    postings.insert(ids.begin(), ids.end());
    if (postings.size() == stored) {
        return;
    }
    const path folder = file.parent_path();
    if (folders.insert(folder.native()).second) {
        files::createDirectories(folder);
    }
    writeAtomically(file, postings);
}

}

FolderPerTermIndexAdapter::FolderPerTermIndexAdapter(path root) : root_(std::move(root)) {
}

void FolderPerTermIndexAdapter::add(const model::TermOccurrences& occurrences) {
    pending_.add(occurrences);
}

void FolderPerTermIndexAdapter::flush() {
    std::unordered_set<path::string_type> folders;
    for (const auto& [term, ids] : pending_.drain()) {
        merge(TermFiles::file(root_, term), ids, folders);
    }
}

}
