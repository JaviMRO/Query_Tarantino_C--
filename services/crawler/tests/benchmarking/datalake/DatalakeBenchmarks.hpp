#pragma once

#include "crawler/tests/benchmarking/support/harness/Benchmark.hpp"

namespace tarantino::crawler::benchmarking {

/// The datalake comparison of SPEC §11, one benchmark class each, as Java's DatalakeWriteBenchmark,
/// DatalakeLookupBenchmark and NewBooksDetectionBenchmark.
class DatalakeWriteBenchmark {
public:
    /// write_throughput: the N books of the dataset ingested as the crawler does, into an empty datalake; then the files,
    /// directories and bytes the datalake takes.
    static Benchmark definition();
};

class DatalakeLookupBenchmark {
public:
    /// lookup_time: the paths of a random book of the dataset.
    static Benchmark definition();
};

class NewBooksDetectionBenchmark {
public:
    /// new_books_detection_time: the 100 new books saved after the N of the dataset, one day older.
    static Benchmark definition();
};

}
