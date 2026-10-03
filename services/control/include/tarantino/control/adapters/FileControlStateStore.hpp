#pragma once

#include "tarantino/control/ports/ControlStateStore.hpp"

#include <filesystem>

namespace tarantino::control::adapters {

/// The control files of SPEC §9 in TARANTINO_CONTROL: downloaded_books.txt and indexed_books.txt, one id per line,
/// append-only. Lines that are not a whole number after stripping are ignored, so a partially written last line is
/// harmless.
class FileControlStateStore : public ports::ControlStateStore {
public:
    explicit FileControlStateStore(const std::filesystem::path& root);

    std::vector<int> downloaded() override;

    std::vector<int> indexed() override;

    void markDownloaded(int bookId) override;

    void markIndexed(int bookId) override;

private:
    std::filesystem::path downloaded_;
    std::filesystem::path indexed_;
};

}
