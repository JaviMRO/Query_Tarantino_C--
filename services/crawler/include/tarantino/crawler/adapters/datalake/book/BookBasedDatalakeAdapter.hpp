#pragma once

#include "tarantino/crawler/ports/DatalakeStorage.hpp"

#include <filesystem>

namespace tarantino::crawler::adapters {

/// The book layout (SPEC §6): <root>/<id>/header.txt and body.txt, one directory per book, found directly by id.
class BookBasedDatalakeAdapter : public ports::DatalakeStorage {
public:
    explicit BookBasedDatalakeAdapter(std::filesystem::path root);

    model::StoredPaths save(const model::BookText& book) override;

    std::optional<model::StoredPaths> pathsOf(int bookId) override;

    std::unordered_set<int> idsStoredSince(platform::Instant instant) override;

    int removeIncompleteWrites() override;

private:
    model::StoredPaths paths(int bookId) const;

    std::filesystem::path root_;
    std::filesystem::path::string_type rootPrefix_;
};

}
