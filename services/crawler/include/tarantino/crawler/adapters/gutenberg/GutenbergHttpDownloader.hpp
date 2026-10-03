#pragma once

#include "tarantino/crawler/ports/BookDownloader.hpp"
#include "tarantino/platform/Time.hpp"

#include <chrono>
#include <functional>
#include <mutex>
#include <optional>
#include <string>

namespace tarantino::crawler::adapters {

/// Downloads books from the official Project Gutenberg mirror of Old Dominion University (SPEC §4), never from
/// www.gutenberg.org, whose robot policy forbids automated access: redirects are followed only within the mirror.
/// The control service downloads several books at once through one downloader, so when the mirror answers that it is
/// busy (429 or 503) every download waits as long as it asks before its next request.
class GutenbergHttpDownloader : public ports::BookDownloader {
public:
    /// How a download waits for a busy mirror: std::this_thread::sleep_for, or a record of the waits in tests.
    using Pause = std::function<void(std::chrono::nanoseconds)>;

    GutenbergHttpDownloader();

    /// A downloader of another server, with the URL of a book written with two %d for its id, for tests.
    GutenbergHttpDownloader(std::string urlTemplate, platform::Clock clock, Pause pause);

    /// Retries while the mirror is busy, waiting what its Retry-After asks or 1, 2, 4, 8 and 16 seconds.
    std::string rawText(int bookId) override;

    /// The answer of the mirror to one request.
    struct Response {
        long status = 0;
        std::string url;
        std::string body;
        std::optional<std::string> location;
        std::optional<std::string> retryAfter;
    };

private:
    void waitForTheMirror();

    void pauseTheMirror(std::chrono::nanoseconds wait);

    Response followingRedirects(const std::string& url, int bookId);

    std::optional<std::chrono::nanoseconds> requestedWait(const Response& response) const;

    std::string urlTemplate_;
    platform::Clock clock_;
    Pause pause_;
    std::mutex pauseMutex_;
    platform::Instant pausedUntil_{};
};

}
