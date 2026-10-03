#include "crawler/tests/benchmarking/support/results/ResultRow.hpp"

#include <cmath>
#include <cstdio>
#include <limits>
#include <stdexcept>
#include <vector>

namespace tarantino::crawler::benchmarking {

namespace {

std::string decimal(double number) {
    char text[64];
    std::snprintf(text, sizeof text, "%.3f", number);
    return text;
}

std::vector<std::string> fields(const std::string& line) {
    std::vector<std::string> split;
    std::size_t start = 0;
    while (true) {
        const std::size_t comma = line.find(',', start);
        split.push_back(line.substr(start, comma == std::string::npos ? std::string::npos : comma - start));
        if (comma == std::string::npos) {
            return split;
        }
        start = comma + 1;
    }
}

bool same(double first, double second) {
    return (std::isnan(first) && std::isnan(second)) || first == second;
}

}

ResultRow ResultRow::exact(std::string structure, std::string metric, int books, double value, std::string unit) {
    return {std::move(structure), std::move(metric), books, value, 0.0, std::move(unit)};
}

ResultRow ResultRow::sample(std::string structure, std::string metric, int books, double value, std::string unit) {
    return {std::move(structure), std::move(metric), books, value, std::numeric_limits<double>::quiet_NaN(), std::move(unit)};
}

ResultRow ResultRow::parse(const std::string& csvLine) {
    const std::vector<std::string> values = fields(csvLine);
    if (values.size() != 7) {
        throw std::invalid_argument("Not a results line: " + csvLine);
    }
    return {values[1], values[2], std::stoi(values[3]), std::stod(values[4]),
            values[5].empty() ? std::numeric_limits<double>::quiet_NaN() : std::stod(values[5]), values[6]};
}

std::string ResultRow::csvLine() const {
    return std::string(LANGUAGE) + "," + structure + "," + metric + "," + std::to_string(books) + "," + decimal(value) + ","
           + (std::isnan(error) ? "" : decimal(error)) + "," + unit;
}

bool ResultRow::operator==(const ResultRow& other) const {
    return structure == other.structure && metric == other.metric && books == other.books && same(value, other.value)
           && same(error, other.error) && unit == other.unit;
}

std::ostream& operator<<(std::ostream& out, const ResultRow& row) {
    return out << row.csvLine();
}

}
