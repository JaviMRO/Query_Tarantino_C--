#include "tarantino/query/adapters/index/folders/FolderPerTermIndexReader.hpp"

#include "adapters/index/folders/TermFiles.hpp"
#include "tarantino/platform/Files.hpp"
#include "tarantino/platform/Paths.hpp"
#include "tarantino/platform/Text.hpp"

#include <stdexcept>

namespace tarantino::query::adapters {

namespace files = platform::files;

FolderPerTermIndexReader::FolderPerTermIndexReader(std::filesystem::path root) : root_(std::move(root)) {
}

ports::InvertedIndexReader::Postings FolderPerTermIndexReader::postings(const std::string& term) {
    const std::filesystem::path file = TermFiles::file(root_, term);
    if (!files::exists(file)) {
        return Postings::none();
    }
    std::unordered_set<int> ids;
    const std::string content = files::readString(file);
    for (const std::string_view line : platform::lines(content)) {
        const auto bookId = platform::parseDigits(line);
        if (!bookId) {
            throw std::runtime_error("Not a book id in " + platform::portable(file) + ": " + std::string(line));
        }
        ids.insert(*bookId);
    }
    return Postings::owning(std::move(ids));
}

}
