#pragma once

#include "tarantino/platform/Paths.hpp"

#include <filesystem>
#include <string>

namespace tarantino::indexer::adapters {

/// Paths as the metadata stores them (SPEC §1): with / as separator on every operating system. Private to the metadata
/// backends.
class PortablePaths {
public:
    PortablePaths() = delete;

    static std::string of(const std::filesystem::path& path) {
        return platform::portable(path);
    }
};

}
