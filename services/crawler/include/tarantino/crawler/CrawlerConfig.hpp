#pragma once

#include <filesystem>
#include <optional>
#include <string>

namespace tarantino::crawler {

/// The configuration of the crawler (SPEC §2). mirror is the local Gutenberg mirror to read books from, or empty to
/// download them over HTTP.
struct CrawlerConfig {
    std::filesystem::path datalake;
    std::string datalakeLayout;
    std::optional<std::filesystem::path> mirror;

    static CrawlerConfig fromEnvironment();
};

}
