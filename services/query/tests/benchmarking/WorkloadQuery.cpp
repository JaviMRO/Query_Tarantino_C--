#include "query/tests/benchmarking/WorkloadQuery.hpp"

#include "tarantino/platform/Text.hpp"

#include <stdexcept>

namespace tarantino::query::benchmarking {

namespace {

constexpr char SEPARATOR = ':';

}

WorkloadQuery WorkloadQuery::parse(std::string_view line) {
    const std::size_t separator = line.find(SEPARATOR);
    if (separator == std::string_view::npos) {
        throw std::invalid_argument("A query must be written as <category>: <query>, found: " + std::string(line));
    }
    return WorkloadQuery{std::string(platform::strip(line.substr(0, separator))), std::string(platform::strip(line.substr(separator + 1)))};
}

}
