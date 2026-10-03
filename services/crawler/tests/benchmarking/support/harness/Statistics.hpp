#pragma once

#include <cstddef>
#include <vector>

namespace tarantino::crawler::benchmarking {

/// The samples of a measurement and what SPEC §11 reports of them, as JMH's ListStatistics: their mean, and the
/// half-width of the confidence interval of the mean, t(1 - (1 - c) / 2, n - 1) · s / √n.
class Statistics {
public:
    Statistics() = default;

    explicit Statistics(std::vector<double> values);

    void add(double value);

    std::size_t count() const;

    double mean() const;

    /// The sample standard deviation, with n - 1 degrees of freedom.
    double standardDeviation() const;

    /// The half-width of the confidence interval of the mean; NaN with fewer than 2 samples, when it cannot be computed.
    double meanErrorAt(double confidence) const;

    /// The smallest value with at least rank percent of the values at or below it (nearest rank, as the histograms of
    /// JMH's SampleTime mode count them); NaN without values.
    double percentile(double rank) const;

    /// The quantile of Student's t distribution with the given degrees of freedom.
    static double studentQuantile(double probability, double degreesOfFreedom);

private:
    std::vector<double> values_;
};

}
