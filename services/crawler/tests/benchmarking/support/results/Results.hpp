#pragma once

#include "crawler/tests/benchmarking/support/harness/Harness.hpp"
#include "crawler/tests/benchmarking/support/results/Metric.hpp"
#include "crawler/tests/benchmarking/support/results/ResultRow.hpp"

#include <map>
#include <string>
#include <vector>

namespace tarantino::crawler::benchmarking {

/// The rows of the measurements (SPEC §11): the value of a metric is the mean of its samples, and its error the
/// half-width of the 95% confidence interval of that mean.
class Results {
public:
    Results() = delete;

    /// The rows of the methods that have a metric, by method name.
    static std::vector<ResultRow> rows(const std::vector<Measurement>& measurements, const std::map<std::string, Metric>& metrics);

    /// A row whose samples are one value per measured iteration of every process, such as the mean or a percentile of
    /// the times sampled during each second.
    static ResultRow perIteration(const Measurement& measurement, const std::string& metric, const std::string& unit,
                                  const std::vector<double>& samples);
};

}
