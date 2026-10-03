#pragma once

#include "tarantino/crawler/ports/BookDownloader.hpp"

#include <functional>
#include <memory>
#include <string>
#include <utility>

namespace tarantino::crawler::ports {

/// A downloader made of a function, as a Java lambda implements BookDownloader: for tests and benchmarks.
class FunctionDownloader : public BookDownloader {
public:
    explicit FunctionDownloader(std::function<std::string(int)> rawText) : rawText_(std::move(rawText)) {
    }

    static std::shared_ptr<BookDownloader> of(std::function<std::string(int)> rawText) {
        return std::make_shared<FunctionDownloader>(std::move(rawText));
    }

    std::string rawText(int bookId) override {
        return rawText_(bookId);
    }

private:
    std::function<std::string(int)> rawText_;
};

}
