#include "crawler/tests/benchmarking/support/dataset/BenchmarkDataset.hpp"

#include "crawler/tests/benchmarking/support/environment/BenchmarkPaths.hpp"
#include "tarantino/platform/Files.hpp"
#include "tarantino/platform/Paths.hpp"
#include "tarantino/platform/Text.hpp"

#include <stdexcept>

namespace tarantino::crawler::benchmarking {

namespace files = platform::files;

namespace {

constexpr int NEW_BOOKS_OFFSET = 1000;

}

BenchmarkDataset::BenchmarkDataset(std::filesystem::path cache, std::filesystem::path bookIds)
    : cache_(std::move(cache)), bookIds_(std::move(bookIds)) {
}

BenchmarkDataset BenchmarkDataset::fromEnvironment() {
    return BenchmarkDataset(BenchmarkPaths::benchmarks() / "cache", BenchmarkPaths::workload() / "book_ids.txt");
}

std::vector<int> BenchmarkDataset::ids(int count) {
    return ids(0, count);
}

std::vector<int> BenchmarkDataset::newIds() {
    return ids(NEW_BOOKS_OFFSET, NEW_BOOKS);
}

std::vector<int> BenchmarkDataset::ids(int offset, int count) {
    const std::vector<int>& ids = cachedIds();
    if (static_cast<std::size_t>(offset + count) > ids.size()) {
        throw std::runtime_error("The cache holds " + std::to_string(ids.size()) + " books but " + std::to_string(offset + count)
                                 + " are needed; run scripts/fill_cache.sh");
    }
    return std::vector<int>(ids.begin() + offset, ids.begin() + offset + count);
}

std::string BenchmarkDataset::rawText(int bookId) const {
    return files::readString(cacheFile(bookId));
}

std::filesystem::path BenchmarkDataset::cacheFile(int bookId) const {
    return cache_ / (std::to_string(bookId) + ".txt");
}

const std::vector<int>& BenchmarkDataset::cachedIds() {
    if (!cachedIds_) {
        std::vector<int> ids;
        for (const std::string& line : files::readLines(bookIds_)) {
            const std::string_view candidate = platform::strip(line);
            if (candidate.empty()) {
                continue;
            }
            const auto bookId = platform::parseDigits(candidate);
            if (!bookId) {
                throw std::runtime_error("Not a book id in " + platform::portable(bookIds_) + ": " + std::string(candidate));
            }
            if (files::exists(cacheFile(*bookId))) {
                ids.push_back(*bookId);
            }
        }
        cachedIds_ = std::move(ids);
    }
    return *cachedIds_;
}

}
