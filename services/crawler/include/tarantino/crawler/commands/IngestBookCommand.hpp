#pragma once

#include "tarantino/crawler/commands/DownloadResult.hpp"
#include "tarantino/crawler/commands/IngestResult.hpp"
#include "tarantino/crawler/ports/BookDownloader.hpp"
#include "tarantino/crawler/ports/DatalakeStorage.hpp"

#include <memory>
#include <optional>

namespace tarantino::crawler::commands {

/// Ingests a book (SPEC §9): looks it up in the datalake and, if it is not there, downloads, splits and stores it.
/// Callers that download several books at once use the three steps apart: downloading writes nothing, so it can run
/// in other threads, while looking up and storing stay in the one thread that uses the datalake.
class IngestBookCommand {
public:
    IngestBookCommand(std::shared_ptr<ports::BookDownloader> downloader, std::shared_ptr<ports::DatalakeStorage> datalake);

    IngestResult execute(int bookId);

    /// The book, if the datalake already holds it: it is not downloaded again.
    std::optional<IngestResult> stored(int bookId);

    DownloadResult download(int bookId);

    /// Stores a downloaded book in the datalake; a failed download stores nothing (SPEC §4).
    IngestResult store(const DownloadResult& download);

private:
    std::shared_ptr<ports::BookDownloader> downloader_;
    std::shared_ptr<ports::DatalakeStorage> datalake_;
};

}
