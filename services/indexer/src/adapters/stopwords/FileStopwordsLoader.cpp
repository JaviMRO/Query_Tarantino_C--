#include "tarantino/indexer/adapters/stopwords/FileStopwordsLoader.hpp"

#include "tarantino/platform/Files.hpp"
#include "tarantino/platform/Unicode.hpp"

namespace tarantino::indexer::adapters {

FileStopwordsLoader::FileStopwordsLoader(std::filesystem::path file) : file_(std::move(file)) {
}

platform::StringSet FileStopwordsLoader::stopwords() {
    platform::StringSet stopwords;
    for (const std::string& line : platform::files::readLines(file_)) {
        std::string stopword = platform::unicode::toLowerCase(platform::strip(line));
        if (!stopword.empty()) {
            stopwords.insert(std::move(stopword));
        }
    }
    return stopwords;
}

}
