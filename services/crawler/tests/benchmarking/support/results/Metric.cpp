#include "crawler/tests/benchmarking/support/results/Metric.hpp"

#include <cmath>

namespace tarantino::crawler::benchmarking {

Metric Metric::asMeasured(std::string name, std::string unit) {
    return {std::move(name), std::move(unit), [](double score, int) { return score; }};
}

Metric Metric::perBook(std::string name, int booksPerRun) {
    return {std::move(name), "ms/book", [booksPerRun](double milliseconds, int) { return milliseconds / booksPerRun; }};
}

Metric Metric::booksPerSecond(std::string name) {
    return {std::move(name), "books/s", [](double milliseconds, int books) { return books / (milliseconds / 1000.0); }};
}

ResultRow Metric::row(const std::string& structure, int books, double score, double scoreError) const {
    const double value = conversion(score, books);
    return ResultRow{structure, name, books, value, std::fabs(value * scoreError / score), unit};
}

}
