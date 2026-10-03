#pragma once

#include "tarantino/crawler/ports/BookDownloader.hpp"
#include "tarantino/crawler/ports/DatalakeStorage.hpp"
#include "tarantino/platform/Time.hpp"

#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace tarantino::crawler::benchmarking {

/// Whether every book ends with exactly one body file, and the files left that are neither header nor body of a stored
/// book (SPEC §11, recovery_ok and recovery_leftover_files).
struct RecoveryOutcome {
    bool recovered;
    std::uint64_t leftoverFiles;
};

/// An interrupted crawl and its resumption (SPEC §11): half of the books are ingested, the next one is interrupted after
/// its header is written, leaving its body as .tmp, and one hour later a new run removes incomplete writes and ingests
/// every book again.
class RecoveryScenario {
public:
    RecoveryScenario(std::string layout, std::filesystem::path root, std::shared_ptr<ports::BookDownloader> downloader);

    RecoveryOutcome run(const std::vector<int>& ids);

private:
    void ingest(const std::vector<int>& ids, platform::Instant runTime);

    void interruptWhileStoring(int bookId);

    bool everyBookStoredOnce(const std::vector<int>& ids);

    std::uint64_t leftoverFiles(const std::vector<int>& ids);

    std::shared_ptr<ports::DatalakeStorage> datalake(platform::Instant runTime) const;

    std::string layout_;
    std::filesystem::path root_;
    std::shared_ptr<ports::BookDownloader> downloader_;
};

}
