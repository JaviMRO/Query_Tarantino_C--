#pragma once

#include "crawler/tests/benchmarking/support/harness/Benchmark.hpp"

namespace tarantino::indexer::benchmarking {

using crawler::benchmarking::Benchmark;

/// The service whose results the indexer benchmarks write: benchmarks/results/cpp-indexer.csv.
inline constexpr const char* INDEXER_SERVICE = "indexer";

/// The structures of the comparisons, mongo only in a build with MongoDB support.
std::vector<std::string> indexStructures();

std::vector<std::string> metadataBackends();

/// full_build_time, memory_allocated, build_memory, term_count, disk_usage and disk_allocated (SPEC §11).
class FullIndexBuildBenchmark {
public:
    static Benchmark definition();
};

/// incremental_update_time and batch_update_time: the new books into an index of exactly the N books of the dataset.
class IncrementalUpdateBenchmark {
public:
    // Flushing after every book is far slower (folders rewrites every term file of each book), so fewer books
    static constexpr int BOOKS_FLUSHED_ONE_BY_ONE = 10;
    static constexpr int BOOKS_FLUSHED_TOGETHER = 100;

    static Benchmark definition();
};

/// bulk_insertion_time: the metadata of N books saved one by one through a single open backend.
class MetadataInsertionBenchmark {
public:
    static Benchmark definition();
};

}
