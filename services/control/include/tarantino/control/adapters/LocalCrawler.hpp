#pragma once

#include "tarantino/control/ports/Crawler.hpp"
#include "tarantino/crawler/commands/IngestBookCommand.hpp"

#include <functional>

namespace tarantino::control::adapters {

/// Looks books up in the caller's thread, downloads them with the given executor and stores them when asked (SPEC §9).
class LocalCrawler : public ports::Crawler {
public:
    /// Runs a task, in another thread or later, as Java's Executor.
    using Executor = std::function<void(std::function<void()>)>;

    LocalCrawler(crawler::commands::IngestBookCommand ingest, Executor downloads);

    void ingest(int bookId, WhenFinished whenFinished) override;

private:
    // Declared first, so that it outlives the executor, whose pool waits for its downloads when destroyed
    crawler::commands::IngestBookCommand ingest_;
    Executor downloads_;
};

}
