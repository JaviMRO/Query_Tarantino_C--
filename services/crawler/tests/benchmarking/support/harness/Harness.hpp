#pragma once

#include "crawler/tests/benchmarking/support/harness/Benchmark.hpp"

#include <optional>
#include <string>
#include <vector>

namespace tarantino::crawler::benchmarking {

/// The samples of one benchmark method, structure and size, from every process of every pass.
struct Measurement {
    std::string benchmark;
    std::string method;
    std::string structure;
    int books;
    /// One per measured iteration, in the time unit of the benchmark.
    std::vector<double> samples;
    /// SampleTime only: the 99th percentile of the calls of each measured iteration.
    std::vector<double> percentiles99;
};

/// The runner of the C++ benchmarks, which does what JMH does for Java (SPEC §11): one operating-system process per
/// benchmark, structure, size and pass, running warm-up iterations, which are discarded, and then measured iterations,
/// timed with std::chrono::steady_clock. The second pass takes the structures and sizes in reverse order.
class Harness {
public:
    Harness() = delete;

    /// As the runner, starts the processes of every pass and returns the measurements; as one of those processes, which
    /// the arguments tell, runs its configuration and returns nothing. A process that fails stops the run.
    static std::optional<std::vector<Measurement>> run(const std::string& service, const std::vector<Benchmark>& benchmarks,
                                                       int argc, char** argv);

    /// The exit status of a benchmark process, once run returned nothing.
    static int processStatus();

    /// Whether the arguments make this process one of the benchmark processes, rather than the runner.
    static bool isProcess(int argc, char** argv);

    /// Records the machine, compiler and libraries the measurements come from (SPEC §11) in
    /// <benchmarks>/<service>/cpp-environment.txt.
    static void recordEnvironment(const std::string& service, const std::vector<std::string>& libraries);
};

}
