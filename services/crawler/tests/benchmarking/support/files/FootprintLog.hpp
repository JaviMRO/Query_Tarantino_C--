#pragma once

#include "crawler/tests/benchmarking/support/results/ResultRow.hpp"

#include <filesystem>
#include <string>
#include <vector>

namespace tarantino::crawler::benchmarking {

/// The measures that benchmark processes take besides their timed iterations, appended to files of the scratch
/// directory and drained by the runner once every process has ended.
class FootprintLog {
public:
    FootprintLog() = delete;

    /// Exact measures, equal in every benchmark process; drained once.
    static void append(const std::string& service, const std::vector<ResultRow>& rows);

    /// One measure per benchmark process or iteration; drained as the mean of all of them, with its confidence interval.
    static void appendSample(const std::string& service, const ResultRow& sample);

    static std::vector<ResultRow> drain(const std::string& service);

    static std::vector<ResultRow> drain(const std::filesystem::path& exactFile, const std::filesystem::path& samplesFile);

    static void appendTo(const std::filesystem::path& file, const std::vector<ResultRow>& rows);
};

}
