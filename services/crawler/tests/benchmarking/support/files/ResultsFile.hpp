#pragma once

#include "crawler/tests/benchmarking/support/results/ResultRow.hpp"

#include <filesystem>
#include <string>
#include <vector>

namespace tarantino::crawler::benchmarking {

/// The results of a service, <benchmarks>/results/cpp-<service>.csv (SPEC §11).
class ResultsFile {
public:
    ResultsFile() = delete;

    static std::filesystem::path write(const std::string& service, const std::vector<ResultRow>& rows);
};

}
