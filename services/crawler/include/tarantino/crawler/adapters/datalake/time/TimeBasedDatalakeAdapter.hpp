#pragma once

#include "tarantino/crawler/ports/DatalakeStorage.hpp"
#include "tarantino/platform/Time.hpp"

#include <filesystem>

namespace tarantino::crawler::adapters {

/// The time layout (SPEC §6): <root>/<YYYYMMDD>/<HH>/<id>.header.txt and .body.txt, by the UTC hour of the save. New
/// books are found by the names of the hour directories; a lookup by id walks the tree.
class TimeBasedDatalakeAdapter : public ports::DatalakeStorage {
public:
    explicit TimeBasedDatalakeAdapter(std::filesystem::path root, platform::Clock clock = platform::systemClock());

    model::StoredPaths save(const model::BookText& book) override;

    std::optional<model::StoredPaths> pathsOf(int bookId) override;

    std::unordered_set<int> idsStoredSince(platform::Instant instant) override;

    int removeIncompleteWrites() override;

private:
    std::filesystem::path currentDirectory() const;

    std::optional<std::filesystem::path> bodyFile(int bookId) const;

    std::filesystem::path root_;
    platform::Clock clock_;
};

}
