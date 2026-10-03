#include "tarantino/control/adapters/LocalCrawler.hpp"

#include "tarantino/platform/Paths.hpp"

namespace tarantino::control::adapters {

using crawler::commands::DownloadResult;
using crawler::commands::IngestResult;

namespace {

model::Outcome outcome(const IngestResult& result) {
    if (result.succeeded()) {
        return model::Outcome::success("stored in " + platform::portable(result.paths->body.parent_path()));
    }
    return model::Outcome::failure("skipped, " + std::string(crawler::model::name(*result.failure)));
}

}

LocalCrawler::LocalCrawler(crawler::commands::IngestBookCommand ingest, Executor downloads)
    : ingest_(std::move(ingest)), downloads_(std::move(downloads)) {
}

void LocalCrawler::ingest(int bookId, WhenFinished whenFinished) {
    if (auto stored = ingest_.stored(bookId)) {
        whenFinished([result = *std::move(stored)] { return outcome(result); }, nullptr);
        return;
    }
    downloads_([this, bookId, whenFinished = std::move(whenFinished)] {
        Download storable;
        std::exception_ptr error;
        try {
            storable = [this, download = ingest_.download(bookId)] { return outcome(ingest_.store(download)); };
        } catch (...) {
            error = std::current_exception();
        }
        whenFinished(std::move(storable), error);
    });
}

}
