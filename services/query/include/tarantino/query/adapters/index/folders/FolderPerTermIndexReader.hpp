#pragma once

#include "tarantino/query/ports/InvertedIndexReader.hpp"

#include <filesystem>

namespace tarantino::query::adapters {

/// Reads the folders structure (SPEC §8.1): the file of the term on every lookup, with nothing kept between them; a
/// missing file is no postings.
class FolderPerTermIndexReader : public ports::InvertedIndexReader {
public:
    explicit FolderPerTermIndexReader(std::filesystem::path root);

    Postings postings(const std::string& term) override;

private:
    std::filesystem::path root_;
};

}
