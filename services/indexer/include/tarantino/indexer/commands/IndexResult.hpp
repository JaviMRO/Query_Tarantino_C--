#pragma once

namespace tarantino::indexer::commands {

/// How many distinct terms of a book were indexed, or that it is not in the datalake.
struct IndexResult {
    int bookId;
    bool indexed;
    int uniqueTerms;

    static IndexResult success(int bookId, int uniqueTerms) {
        return {bookId, true, uniqueTerms};
    }

    static IndexResult notFound(int bookId) {
        return {bookId, false, 0};
    }

    bool operator==(const IndexResult&) const = default;
};

}
