#pragma once

#include "tarantino/indexer/ports/sources/StopwordsLoader.hpp"

#include <filesystem>

namespace tarantino::indexer::adapters {

/// The stopwords of a file, one per line, each stripped and lowercased as the tokenizer lowercases (SPEC §3); blank
/// lines are ignored.
class FileStopwordsLoader : public ports::StopwordsLoader {
public:
    explicit FileStopwordsLoader(std::filesystem::path file);

    platform::StringSet stopwords() override;

private:
    std::filesystem::path file_;
};

}
