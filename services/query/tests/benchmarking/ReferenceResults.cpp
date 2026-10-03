#include "query/tests/benchmarking/ReferenceResults.hpp"

#include "tarantino/platform/Files.hpp"

#include <algorithm>
#include <stdexcept>

namespace tarantino::query::benchmarking {

namespace files = platform::files;

namespace {

constexpr char QUERY_SEPARATOR = '\t';
constexpr char ID_SEPARATOR = ',';

std::vector<int> ids(const std::string& text) {
    std::vector<int> ids;
    std::size_t start = 0;
    while (start < text.size()) {
        const std::size_t end = std::min(text.find(ID_SEPARATOR, start), text.size());
        ids.push_back(std::stoi(text.substr(start, end - start)));
        start = end + 1;
    }
    return ids;
}

}

std::map<std::string, std::vector<int>> ReferenceResults::read(const std::filesystem::path& file) {
    std::map<std::string, std::vector<int>> results;
    for (const std::string& line : files::readLines(file)) {
        const std::size_t separator = line.find(QUERY_SEPARATOR);
        if (separator == std::string::npos) {
            throw std::runtime_error("Malformed line of reference results: " + line);
        }
        results.emplace(line.substr(0, separator), ids(line.substr(separator + 1)));
    }
    return results;
}

void ReferenceResults::write(const std::filesystem::path& file, const std::map<std::string, std::vector<int>>& results) {
    std::string text;
    for (const auto& [query, ids] : results) {
        text += query;
        text += QUERY_SEPARATOR;
        for (std::size_t id = 0; id < ids.size(); ++id) {
            if (id > 0) {
                text += ID_SEPARATOR;
            }
            text += std::to_string(ids[id]);
        }
        text += '\n';
    }
    files::createDirectories(file.parent_path());
    files::writeString(file, text);
}

}
