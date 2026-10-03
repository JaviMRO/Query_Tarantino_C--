#pragma once

#include <nlohmann/json.hpp>

#include <cstddef>
#include <filesystem>
#include <optional>
#include <string>
#include <unordered_set>
#include <vector>

namespace tarantino::crawler::conformance {

/// The language-neutral cases of TARANTINO_WORKLOAD/conformance (SPEC §13), which every implementation must pass.
/// Without the variable, workload/conformance is looked for from the working directory upwards, so the tests run from
/// the project root, from a build directory inside it and from an IDE alike.
class ConformanceCases {
public:
    ConformanceCases() = delete;

    static std::filesystem::path directory();

    static nlohmann::json file(const std::string& name);

    /// The cases of a file; none, with the reason on standard error, when it cannot be read.
    static std::vector<nlohmann::json> cases(const std::string& fileName);

    static std::unordered_set<std::string> stopwords(const nlohmann::json& file);

    static std::optional<std::string> textOrNull(const nlohmann::json& node);

    /// The name GoogleTest reports a case by: its position and its name with every other character than ASCII letters
    /// and digits written as _, since GoogleTest accepts no other.
    static std::string testName(std::size_t index, const std::string& name);
};

}
