#pragma once

#include <filesystem>
#include <map>
#include <string>
#include <vector>

namespace tarantino::query::benchmarking {

/// The expected ids of every query, one <query>TAB<id>,<id>,... line each, so that the processes of a run compute them
/// once.
class ReferenceResults {
public:
    ReferenceResults() = delete;

    static std::map<std::string, std::vector<int>> read(const std::filesystem::path& file);

    static void write(const std::filesystem::path& file, const std::map<std::string, std::vector<int>>& results);
};

}
