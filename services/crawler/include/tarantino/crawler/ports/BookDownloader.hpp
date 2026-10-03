#pragma once

#include <string>

namespace tarantino::crawler::ports {

/// Where books come from (SPEC §4). The control layer downloads several books at once through one downloader, so it
/// must accept calls from several threads.
class BookDownloader {
public:
    virtual ~BookDownloader() = default;

    /// The raw text of the book, as UTF-8; throws model::DownloadException with the reason when it cannot be had.
    virtual std::string rawText(int bookId) = 0;
};

}
