#include "tarantino/crawler/CrawlerConfig.hpp"

#include "tarantino/platform/Environment.hpp"
#include "tarantino/platform/Paths.hpp"
#include "tarantino/platform/Text.hpp"

namespace tarantino::crawler {

CrawlerConfig CrawlerConfig::fromEnvironment() {
    const std::string mirror = platform::environmentVariable("TARANTINO_MIRROR", "");
    return {platform::pathOf(platform::environmentVariable("TARANTINO_DATALAKE", "datalake")),
            platform::environmentVariable("TARANTINO_DATALAKE_LAYOUT", "time"),
            platform::strip(mirror).empty() ? std::nullopt : std::optional(platform::pathOf(mirror))};
}

}
