#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace tarantino::crawler::benchmarking {

/// The cached books of SPEC §11: the ids of book_ids.txt that <benchmarks>/cache holds, in that order (the cache order).
class BenchmarkDataset {
public:
    /// The 100 new books of the update and detection benchmarks: positions 1001 to 1100 of the cache order.
    static constexpr int NEW_BOOKS = 100;

    BenchmarkDataset(std::filesystem::path cache, std::filesystem::path bookIds);

    static BenchmarkDataset fromEnvironment();

    /// The dataset of size count: the first count ids of the cache order.
    std::vector<int> ids(int count);

    std::vector<int> newIds();

    std::vector<int> ids(int offset, int count);

    /// The raw text of a cached book, as UTF-8.
    std::string rawText(int bookId) const;

private:
    std::filesystem::path cacheFile(int bookId) const;

    const std::vector<int>& cachedIds();

    std::filesystem::path cache_;
    std::filesystem::path bookIds_;
    std::optional<std::vector<int>> cachedIds_;
};

}
