#pragma once

#include "tarantino/platform/Text.hpp"

#include <filesystem>
#include <unordered_set>

namespace tarantino::query::adapters {

/// The file of the json structure, {"term": [ids], ...} (SPEC §8.1), parsed with nlohmann/json's SAX interface
/// straight into a hash map from term to a hash set of ids, as Jackson builds Java's HashMap<String, HashSet<Integer>>.
/// Private to the json structure.
class JsonIndexFile {
public:
    JsonIndexFile() = delete;

    static platform::StringMap<std::unordered_set<int>> read(const std::filesystem::path& file);
};

}
