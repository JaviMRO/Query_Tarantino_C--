#pragma once

#include "tarantino/crawler/ports/BookDownloader.hpp"

#include <filesystem>
#include <vector>

namespace tarantino::crawler::adapters {

/// Reads books from a local copy of Project Gutenberg's generated collection, made in bulk with rsync (SPEC §4),
/// instead of one HTTP request per book. Same files, same failures: a missing book is NOT_FOUND and any other I/O
/// error NETWORK_ERROR.
class LocalMirrorDownloader : public ports::BookDownloader {
public:
    explicit LocalMirrorDownloader(std::filesystem::path mirror);

    std::string rawText(int bookId) override;

    /// The books of the mirror in ascending id order: the directories named by a book id that hold its text.
    std::vector<int> bookIds() const;

private:
    std::filesystem::path file(int bookId) const;

    std::filesystem::path mirror_;
};

}
