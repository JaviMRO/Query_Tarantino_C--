#include "crawler/tests/benchmarking/support/files/ResultsFile.hpp"

#include "crawler/tests/benchmarking/support/environment/BenchmarkPaths.hpp"
#include "tarantino/platform/Files.hpp"

namespace tarantino::crawler::benchmarking {

std::filesystem::path ResultsFile::write(const std::string& service, const std::vector<ResultRow>& rows) {
    const std::filesystem::path file =
            BenchmarkPaths::benchmarks() / "results" / (std::string(ResultRow::LANGUAGE) + "-" + service + ".csv");
    platform::files::createDirectories(file.parent_path());
    std::string content = std::string(ResultRow::CSV_HEADER) + "\n";
    for (const ResultRow& row : rows) {
        content += row.csvLine() + "\n";
    }
    platform::files::writeString(file, content);
    return file;
}

}
