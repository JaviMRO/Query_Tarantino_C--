#pragma once

#include "crawler/tests/benchmarking/support/dataset/BenchmarkDataset.hpp"
#include "crawler/tests/benchmarking/support/results/ResultRow.hpp"
#include "tarantino/crawler/model/book/StoredPaths.hpp"
#include "tarantino/crawler/ports/DatalakeStorage.hpp"
#include "tarantino/platform/Time.hpp"

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace tarantino::crawler::benchmarking {

/// Datalakes of the dataset, written as the crawler writes them.
class DatalakeFixture {
public:
    static const std::vector<std::string> LAYOUTS;
    static const platform::Instant CRAWL_START;

    DatalakeFixture() = delete;

    static std::shared_ptr<ports::DatalakeStorage> datalake(const std::string& layout, const std::filesystem::path& root,
                                                            platform::Clock clock = platform::systemClock());

    /// Ingests the books as the crawler does: skipping stored ones, then reading, splitting and saving (SPEC §9).
    static std::vector<model::StoredPaths> ingest(const std::shared_ptr<ports::DatalakeStorage>& datalake,
                                                  const std::vector<int>& ids, BenchmarkDataset& dataset);

    /// Ingests the books as a crawl of 100 books per hour from start saves them (SPEC §11).
    static std::vector<model::StoredPaths> ingestAsCrawled(const std::string& layout, const std::filesystem::path& root,
                                                           const std::vector<int>& ids, BenchmarkDataset& dataset,
                                                           platform::Instant start);

    static std::vector<ResultRow> footprint(const std::string& layout, int books, const std::filesystem::path& root);
};

}
