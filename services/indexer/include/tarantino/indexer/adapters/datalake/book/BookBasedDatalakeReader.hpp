#pragma once

#include "tarantino/indexer/ports/sources/DatalakeReader.hpp"

#include <filesystem>

namespace tarantino::indexer::adapters {

/// Reads the book layout (SPEC §6): <root>/<id>/header.txt and body.txt.
class BookBasedDatalakeReader : public ports::DatalakeReader {
public:
    explicit BookBasedDatalakeReader(std::filesystem::path root);

    std::optional<model::BookText> bookText(int bookId) override;

private:
    std::filesystem::path root_;
};

}
