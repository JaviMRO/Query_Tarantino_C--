#pragma once

#include <filesystem>
#include <map>
#include <set>
#include <string>

namespace tarantino::indexer::adapters {

/// The file of the json structure, {"term": [ids], ...} (SPEC §8.1), read and written as Jackson does: parsed with
/// nlohmann/json's SAX interface straight into the index, with no document in between, and written as a stream.
/// Private to the json structure.
class JsonIndexFile {
public:
    JsonIndexFile() = delete;

    static std::map<std::string, std::set<int>> read(const std::filesystem::path& file);

    /// Writes {"island":[5,1342],"whale":[1342]}, the compact form Jackson writes.
    static void write(const std::filesystem::path& file, const std::map<std::string, std::set<int>>& index);
};

}
