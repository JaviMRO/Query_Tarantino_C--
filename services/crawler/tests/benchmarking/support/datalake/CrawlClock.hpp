#pragma once

#include "tarantino/platform/Time.hpp"

#include <chrono>

namespace tarantino::crawler::benchmarking {

/// The clock of a crawl that downloads 100 books per hour (SPEC §11): the book at position i is saved at
/// start + ⌊i / 100⌋ hours.
class CrawlClock {
public:
    static constexpr int BOOKS_PER_HOUR = 100;

    explicit CrawlClock(platform::Instant start) : start_(start) {
    }

    static platform::Instant instantOf(platform::Instant start, int position) {
        return start + std::chrono::hours(position / BOOKS_PER_HOUR);
    }

    /// The hours the crawl of the books takes, ⌈books / 100⌉.
    static std::chrono::hours durationOf(int books) {
        return std::chrono::hours((books + BOOKS_PER_HOUR - 1) / BOOKS_PER_HOUR);
    }

    void moveTo(int position) {
        position_ = position;
    }

    /// The clock as the datalake reads it; it follows every later moveTo.
    platform::Clock clock() const {
        return [this] { return instantOf(start_, position_); };
    }

private:
    platform::Instant start_;
    int position_ = 0;
};

}
