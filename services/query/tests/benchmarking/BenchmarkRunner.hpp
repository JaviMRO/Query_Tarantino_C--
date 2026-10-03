#pragma once

#include "crawler/tests/benchmarking/support/harness/Benchmark.hpp"

namespace tarantino::query::benchmarking {

using crawler::benchmarking::Benchmark;

/// The service whose results the query benchmarks write: benchmarks/results/cpp-query.csv.
inline constexpr const char* QUERY_SERVICE = "query";
inline constexpr const char* QUERY_TIME_UNIT = "\xC2\xB5s/query";

/// The structures of the comparisons, mongo only in a build with MongoDB support.
std::vector<std::string> indexStructures();

std::vector<std::string> metadataBackends();

/// index_open_time: a search opened over an index of N books and its first query answered.
class IndexOpenBenchmark {
public:
    static Benchmark definition();
};

/// book_by_id_time and books_by_author_time: a random book or author of the N books, through an open reader.
class MetadataQueryBenchmark {
public:
    static Benchmark definition();
};

/// query_time, query_time_p99, query_time_<category> and index_memory (SPEC §11), with every query checked first.
class QueryTimeBenchmark {
public:
    static Benchmark definition();
};

}
