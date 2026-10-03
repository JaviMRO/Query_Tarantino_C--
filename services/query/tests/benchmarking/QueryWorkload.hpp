#pragma once

#include "query/tests/benchmarking/WorkloadQuery.hpp"
#include "tarantino/platform/Text.hpp"
#include "tarantino/query/commands/SearchCommand.hpp"

#include <string>
#include <vector>

namespace tarantino::indexer::benchmarking {
class BenchmarkStore;
}

namespace tarantino::query::benchmarking {

/// The queries of the workload and the search that answers them over an index of the benchmarks (SPEC §11).
class QueryWorkload {
public:
    static constexpr const char* ALL_CATEGORIES = "all";

    QueryWorkload() = delete;

    /// A search over the index of the store whose metadata is constant: the index benchmarks measure only the index.
    static commands::SearchCommand openSearch(const indexer::benchmarking::BenchmarkStore& store, const std::string& index,
                                              const platform::StringSet& stopwords);

    static platform::StringSet stopwords();

    static std::vector<WorkloadQuery> queries();

    /// The texts of the queries of a category, or of every query for ALL_CATEGORIES, in the order of the workload.
    static std::vector<std::string> texts(const std::vector<WorkloadQuery>& queries, const std::string& category);
};

}
