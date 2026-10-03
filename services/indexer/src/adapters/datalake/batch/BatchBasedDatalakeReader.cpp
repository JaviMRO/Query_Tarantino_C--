#include "tarantino/indexer/adapters/datalake/batch/BatchBasedDatalakeReader.hpp"

#include "tarantino/indexer/adapters/datalake/BookFiles.hpp"

#include <string>

namespace tarantino::indexer::adapters {

namespace {

constexpr int BATCH_SIZE = 1000;

}

BatchBasedDatalakeReader::BatchBasedDatalakeReader(std::filesystem::path root) : root_(std::move(root)) {
}

std::optional<model::BookText> BatchBasedDatalakeReader::bookText(int bookId) {
    const std::filesystem::path directory = root_ / std::to_string(bookId / BATCH_SIZE);
    const std::string id = std::to_string(bookId);
    return BookFiles::bookText(bookId, directory / (id + ".header.txt"), directory / (id + ".body.txt"));
}

}
