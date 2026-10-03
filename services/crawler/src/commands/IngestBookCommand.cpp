#include "tarantino/crawler/commands/IngestBookCommand.hpp"

#include "tarantino/crawler/model/book/GutenbergText.hpp"
#include "tarantino/crawler/model/failure/DownloadException.hpp"

#include <filesystem>

namespace tarantino::crawler::commands {

IngestBookCommand::IngestBookCommand(std::shared_ptr<ports::BookDownloader> downloader,
                                     std::shared_ptr<ports::DatalakeStorage> datalake)
    : downloader_(std::move(downloader)), datalake_(std::move(datalake)) {
}

IngestResult IngestBookCommand::execute(int bookId) {
    if (auto stored = this->stored(bookId)) {
        return *std::move(stored);
    }
    return store(download(bookId));
}

std::optional<IngestResult> IngestBookCommand::stored(int bookId) {
    if (auto paths = datalake_->pathsOf(bookId)) {
        return IngestResult::success(bookId, *std::move(paths));
    }
    return std::nullopt;
}

DownloadResult IngestBookCommand::download(int bookId) {
    try {
        return DownloadResult::success(model::GutenbergText::bookText(bookId, downloader_->rawText(bookId)));
    } catch (const model::DownloadException& failure) {
        return DownloadResult::failed(bookId, failure.reason());
    }
}

IngestResult IngestBookCommand::store(const DownloadResult& download) {
    if (!download.succeeded()) {
        return IngestResult::failed(download.bookId, *download.failure);
    }
    try {
        return IngestResult::success(download.bookId, datalake_->save(*download.text));
    } catch (const std::filesystem::filesystem_error&) {
        return IngestResult::failed(download.bookId, model::FailureReason::StorageError);
    }
}

}
