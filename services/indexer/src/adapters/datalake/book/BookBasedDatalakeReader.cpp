#include "tarantino/indexer/adapters/datalake/book/BookBasedDatalakeReader.hpp"

#include "tarantino/indexer/adapters/datalake/BookFiles.hpp"

#include <string>

namespace tarantino::indexer::adapters {

BookBasedDatalakeReader::BookBasedDatalakeReader(std::filesystem::path root) : root_(std::move(root)) {
}

std::optional<model::BookText> BookBasedDatalakeReader::bookText(int bookId) {
    const std::filesystem::path directory = root_ / std::to_string(bookId);
    return BookFiles::bookText(bookId, directory / "header.txt", directory / "body.txt");
}

}
