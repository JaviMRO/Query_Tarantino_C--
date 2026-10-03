#pragma once

#include "tarantino/platform/Files.hpp"
#include "tarantino/platform/Paths.hpp"

#include <atomic>
#include <chrono>
#include <filesystem>
#include <random>
#include <string>
#include <string_view>

namespace tarantino::testing {

/// A directory of its own for a test, deleted with everything in it when the test ends, as JUnit's @TempDir.
class TempDirectory {
public:
    TempDirectory() {
        static std::atomic<unsigned> created{0};
        std::mt19937_64 random(std::random_device{}() ^ static_cast<std::uint64_t>(
                std::chrono::steady_clock::now().time_since_epoch().count()) ^ ++created);
        path_ = std::filesystem::temp_directory_path() / ("tarantino-test-" + std::to_string(random()));
        platform::files::createDirectories(path_);
    }

    TempDirectory(const TempDirectory&) = delete;
    TempDirectory& operator=(const TempDirectory&) = delete;

    ~TempDirectory() {
        try {
            platform::files::removeAll(path_);
        } catch (const std::exception&) {
            // A file still open elsewhere; the system's temporary directory is cleaned up sooner or later
        }
    }

    const std::filesystem::path& path() const {
        return path_;
    }

    /// A path inside the directory, written with / as separator.
    std::filesystem::path operator/(std::string_view relative) const {
        return path_ / platform::pathOf(relative);
    }

private:
    std::filesystem::path path_;
};

}
