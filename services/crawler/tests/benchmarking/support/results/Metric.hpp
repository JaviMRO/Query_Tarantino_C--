#pragma once

#include "crawler/tests/benchmarking/support/results/ResultRow.hpp"

#include <functional>
#include <string>

namespace tarantino::crawler::benchmarking {

/// How the score of a benchmark method, in the time unit it is measured in, becomes a metric of SPEC §11.
struct Metric {
    using Conversion = std::function<double(double score, int books)>;

    std::string name;
    std::string unit;
    Conversion conversion;

    static Metric asMeasured(std::string name, std::string unit);

    /// The time of a run of booksPerRun books, in ms, per book.
    static Metric perBook(std::string name, int booksPerRun);

    /// The books of the run divided by its time in ms, in books per second.
    static Metric booksPerSecond(std::string name);

    /// The row of the converted score, whose error keeps the relative error of the score (SPEC §11).
    ResultRow row(const std::string& structure, int books, double score, double scoreError) const;
};

}
