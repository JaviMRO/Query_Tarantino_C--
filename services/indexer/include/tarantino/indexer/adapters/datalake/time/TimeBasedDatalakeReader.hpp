#pragma once

#include "tarantino/indexer/ports/sources/DatalakeReader.hpp"

#include <filesystem>

namespace tarantino::indexer::adapters {

/// Reads the time layout (SPEC §6), walking the tree to the first body file of the book.
class TimeBasedDatalakeReader : public ports::DatalakeReader {
public:
    explicit TimeBasedDatalakeReader(std::filesystem::path root);

    std::optional<model::BookText> bookText(int bookId) override;

private:
    std::filesystem::path root_;
};

}
