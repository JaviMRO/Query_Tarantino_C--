#include "crawler/tests/benchmarking/support/datalake/RecoveryScenario.hpp"

#include "crawler/tests/benchmarking/support/datalake/DatalakeFixture.hpp"
#include "crawler/tests/benchmarking/support/files/Directories.hpp"
#include "tarantino/crawler/commands/IngestBookCommand.hpp"
#include "tarantino/crawler/model/book/GutenbergText.hpp"
#include "tarantino/platform/Files.hpp"
#include "tarantino/platform/Paths.hpp"

#include <set>

namespace tarantino::crawler::benchmarking {

namespace {

const platform::Instant INTERRUPTED_RUN = *platform::parseInstant("2025-09-25T14:00:00Z");
const platform::Instant RESUMED_RUN = INTERRUPTED_RUN + std::chrono::hours(1);

bool isBody(const std::filesystem::path& file) {
    const std::string name = platform::portable(file.filename());
    return name == "body.txt" || name.ends_with(".body.txt");
}

}

RecoveryScenario::RecoveryScenario(std::string layout, std::filesystem::path root, std::shared_ptr<ports::BookDownloader> downloader)
    : layout_(std::move(layout)), root_(std::move(root)), downloader_(std::move(downloader)) {
}

RecoveryOutcome RecoveryScenario::run(const std::vector<int>& ids) {
    const std::size_t interruptedBook = ids.size() / 2;
    ingest(std::vector<int>(ids.begin(), ids.begin() + static_cast<std::ptrdiff_t>(interruptedBook)), INTERRUPTED_RUN);
    interruptWhileStoring(ids[interruptedBook]);
    datalake(RESUMED_RUN)->removeIncompleteWrites();
    ingest(ids, RESUMED_RUN);
    return RecoveryOutcome{everyBookStoredOnce(ids), leftoverFiles(ids)};
}

void RecoveryScenario::ingest(const std::vector<int>& ids, platform::Instant runTime) {
    commands::IngestBookCommand ingest(downloader_, datalake(runTime));
    for (const int bookId : ids) {
        ingest.execute(bookId);
    }
}

void RecoveryScenario::interruptWhileStoring(int bookId) {
    const model::StoredPaths paths = datalake(INTERRUPTED_RUN)->save(model::GutenbergText::bookText(bookId, downloader_->rawText(bookId)));
    std::filesystem::path temporary = paths.body;
    temporary += ".tmp";
    platform::files::move(paths.body, temporary);
}

bool RecoveryScenario::everyBookStoredOnce(const std::vector<int>& ids) {
    const auto datalake = this->datalake(RESUMED_RUN);
    if (Directories::fileCount(root_, isBody) != ids.size()) {
        return false;
    }
    for (const int bookId : ids) {
        if (!datalake->pathsOf(bookId)) {
            return false;
        }
    }
    return true;
}

std::uint64_t RecoveryScenario::leftoverFiles(const std::vector<int>& ids) {
    const auto datalake = this->datalake(RESUMED_RUN);
    std::set<std::filesystem::path> bookFiles;
    for (const int bookId : ids) {
        if (const auto paths = datalake->pathsOf(bookId)) {
            bookFiles.insert(paths->header);
            bookFiles.insert(paths->body);
        }
    }
    return Directories::fileCount(root_, [&](const std::filesystem::path& file) { return !bookFiles.contains(file); });
}

std::shared_ptr<ports::DatalakeStorage> RecoveryScenario::datalake(platform::Instant runTime) const {
    return DatalakeFixture::datalake(layout_, root_, platform::fixedClock(runTime));
}

}
