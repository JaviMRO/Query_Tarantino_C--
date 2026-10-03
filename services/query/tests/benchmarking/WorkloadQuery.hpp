#pragma once

#include <string>
#include <string_view>

namespace tarantino::query::benchmarking {

/// A line of queries.txt: <category>: <query> (SPEC §3).
struct WorkloadQuery {
    std::string category;
    std::string text;

    static WorkloadQuery parse(std::string_view line);

    bool operator==(const WorkloadQuery&) const = default;
};

}
