#include "crawler/tests/conformance/ConformanceCases.hpp"

#include "tarantino/platform/Environment.hpp"
#include "tarantino/platform/Files.hpp"
#include "tarantino/platform/Paths.hpp"
#include "tarantino/platform/Text.hpp"

#include <iostream>
#include <stdexcept>

namespace tarantino::crawler::conformance {

std::filesystem::path ConformanceCases::directory() {
    const auto workload = platform::environmentVariable("TARANTINO_WORKLOAD");
    if (workload && !platform::strip(*workload).empty()) {
        return platform::pathOf(*workload) / "conformance";
    }
    for (std::filesystem::path directory = std::filesystem::current_path(); !directory.empty();
         directory = directory.parent_path()) {
        const std::filesystem::path candidate = directory / "workload" / "conformance";
        if (platform::files::isDirectory(candidate)) {
            return candidate;
        }
        if (directory == directory.parent_path()) {
            break;
        }
    }
#ifdef TARANTINO_SOURCE_WORKLOAD
    // The workload of the sources the tests were built from, for a build directory outside the project
    const std::filesystem::path source = platform::pathOf(TARANTINO_SOURCE_WORKLOAD) / "conformance";
    if (platform::files::isDirectory(source)) {
        return source;
    }
#endif
    throw std::runtime_error("workload/conformance not found from " + platform::portable(std::filesystem::current_path())
                             + "; set TARANTINO_WORKLOAD");
}

nlohmann::json ConformanceCases::file(const std::string& name) {
    return nlohmann::json::parse(platform::files::readString(directory() / name));
}

std::vector<nlohmann::json> ConformanceCases::cases(const std::string& fileName) {
    try {
        const nlohmann::json json = file(fileName);
        return std::vector<nlohmann::json>(json.at("cases").begin(), json.at("cases").end());
    } catch (const std::exception& error) {
        std::cerr << "Could not read the conformance cases of " << fileName << ": " << error.what() << std::endl;
        return {};
    }
}

std::unordered_set<std::string> ConformanceCases::stopwords(const nlohmann::json& file) {
    std::unordered_set<std::string> stopwords;
    for (const auto& stopword : file.at("stopwords")) {
        stopwords.insert(stopword.get<std::string>());
    }
    return stopwords;
}

std::optional<std::string> ConformanceCases::textOrNull(const nlohmann::json& node) {
    if (node.is_null()) {
        return std::nullopt;
    }
    return node.get<std::string>();
}

std::string ConformanceCases::testName(std::size_t index, const std::string& name) {
    std::string testName = std::to_string(index) + "_";
    for (const char character : name) {
        const bool alphanumeric = (character >= 'a' && character <= 'z') || (character >= 'A' && character <= 'Z')
                                  || (character >= '0' && character <= '9');
        testName.push_back(alphanumeric ? character : '_');
    }
    return testName;
}

}
