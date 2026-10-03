#pragma once

#include "tarantino/indexer/model/book/BookText.hpp"

#include <optional>

namespace tarantino::indexer::ports {

/// Reads books from the datalake of SPEC §6, in the layout the crawler wrote it.
class DatalakeReader {
public:
    virtual ~DatalakeReader() = default;

    /// The book, if the datalake holds its body.
    virtual std::optional<model::BookText> bookText(int bookId) = 0;
};

}
