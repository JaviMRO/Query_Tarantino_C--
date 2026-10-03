#include "tarantino/query/adapters/index/json/MonolithicJsonIndexReader.hpp"

#include "adapters/index/json/JsonIndexFile.hpp"
#include "tarantino/platform/Files.hpp"

namespace tarantino::query::adapters {

MonolithicJsonIndexReader::MonolithicJsonIndexReader(std::filesystem::path file) : file_(std::move(file)) {
}

ports::InvertedIndexReader::Postings MonolithicJsonIndexReader::postings(const std::string& term) {
    const auto& stored = index();
    const auto found = stored.find(term);
    return found == stored.end() ? Postings::none() : Postings::of(found->second);
}

const platform::StringMap<std::unordered_set<int>>& MonolithicJsonIndexReader::index() {
    if (!index_) {
        index_ = platform::files::exists(file_) ? JsonIndexFile::read(file_) : platform::StringMap<std::unordered_set<int>>{};
    }
    return *index_;
}

}
