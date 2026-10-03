#pragma once

#include <ostream>
#include <string>

namespace tarantino::crawler::benchmarking {

/// A row of the results file of SPEC §11: language,structure,metric,n_books,value,error,unit.
struct ResultRow {
    static constexpr const char* LANGUAGE = "cpp";
    static constexpr const char* CSV_HEADER = "language,structure,metric,n_books,value,error,unit";
    /// error is the half-width of this confidence interval of the mean of the samples (SPEC §11).
    static constexpr double CONFIDENCE = 0.95;

    std::string structure;
    std::string metric;
    int books;
    double value;
    /// NaN when it cannot be computed (fewer than 2 samples); 0 for exact values.
    double error;
    std::string unit;

    static ResultRow exact(std::string structure, std::string metric, int books, double value, std::string unit);

    /// One sample of a measure taken once per process, whose error is computed when the samples are drained.
    static ResultRow sample(std::string structure, std::string metric, int books, double value, std::string unit);

    static ResultRow parse(const std::string& csvLine);

    /// The line with three decimals and an empty error when it is unknown, e.g. cpp,time,write_throughput,1000,812.400,35.250,books/s
    std::string csvLine() const;

    bool operator==(const ResultRow& other) const;
};

std::ostream& operator<<(std::ostream& out, const ResultRow& row);

}
