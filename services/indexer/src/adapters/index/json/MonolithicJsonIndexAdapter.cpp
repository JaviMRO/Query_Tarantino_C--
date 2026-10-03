#include "tarantino/indexer/adapters/index/json/MonolithicJsonIndexAdapter.hpp"

#include "adapters/index/json/JsonIndexFile.hpp"
#include "tarantino/platform/Files.hpp"

namespace tarantino::indexer::adapters {

namespace files = platform::files;

MonolithicJsonIndexAdapter::MonolithicJsonIndexAdapter(std::filesystem::path file) : file_(std::move(file)) {
}

void MonolithicJsonIndexAdapter::open() {
    index();
}

void MonolithicJsonIndexAdapter::add(const model::TermOccurrences& occurrences) {
    auto& stored = index();
    for (const auto& [term, frequency] : occurrences.frequencies) {
        stored[term].insert(occurrences.bookId);
    }
}

void MonolithicJsonIndexAdapter::flush() {
    files::createDirectories(std::filesystem::absolute(file_).parent_path());
    std::filesystem::path temporary = file_;
    temporary += ".tmp";
    JsonIndexFile::write(temporary, index());
    files::move(temporary, file_);
}

std::map<std::string, std::set<int>>& MonolithicJsonIndexAdapter::index() {
    if (!index_) {
        index_ = files::exists(file_) ? JsonIndexFile::read(file_) : std::map<std::string, std::set<int>>{};
    }
    return *index_;
}

}
