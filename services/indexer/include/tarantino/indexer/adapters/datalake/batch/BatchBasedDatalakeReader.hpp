#pragma once

#include "tarantino/indexer/ports/sources/DatalakeReader.hpp"

#include <filesystem>

namespace tarantino::indexer::adapters {

/// Reads the batch layout (SPEC §6): <root>/<id div 1000>/<id>.header.txt and .body.txt.
class BatchBasedDatalakeReader : public ports::DatalakeReader {
public:
    explicit BatchBasedDatalakeReader(std::filesystem::path root);

    std::optional<model::BookText> bookText(int bookId) override;

private:
    std::filesystem::path root_;
};

}
