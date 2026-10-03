#include "crawler/tests/benchmarking/support/results/Results.hpp"

#include "crawler/tests/benchmarking/support/harness/Statistics.hpp"

namespace tarantino::crawler::benchmarking {

std::vector<ResultRow> Results::rows(const std::vector<Measurement>& measurements, const std::map<std::string, Metric>& metrics) {
    std::vector<ResultRow> rows;
    for (const Measurement& measurement : measurements) {
        const auto metric = metrics.find(measurement.method);
        if (metric == metrics.end()) {
            continue;
        }
        const Statistics statistics(measurement.samples);
        rows.push_back(metric->second.row(measurement.structure, measurement.books, statistics.mean(),
                                          statistics.meanErrorAt(ResultRow::CONFIDENCE)));
    }
    return rows;
}

ResultRow Results::perIteration(const Measurement& measurement, const std::string& metric, const std::string& unit,
                                const std::vector<double>& samples) {
    const Statistics statistics(samples);
    return ResultRow{measurement.structure, metric, measurement.books, statistics.mean(),
                     statistics.meanErrorAt(ResultRow::CONFIDENCE), unit};
}

}
