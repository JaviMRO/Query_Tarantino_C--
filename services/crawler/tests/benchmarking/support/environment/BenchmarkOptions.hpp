#pragma once

#include <chrono>
#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace tarantino::crawler::benchmarking {

/// Warm-up and measured iterations of a process, and how long each one lasts when it repeats an operation.
struct Iterations {
    int warmup;
    int measurement;
    std::chrono::milliseconds time;
};

/// A pass over every benchmark of a service, with one process per configuration (SPEC §11).
struct Pass {
    int number;
    /// The second pass takes the structures and the sizes in reverse order.
    bool reversed;
};

/// The benchmarks to run, from the environment: TARANTINO_BENCHMARK_BOOKS (sizes such as 100,300),
/// TARANTINO_BENCHMARK_QUICK (true: one pass of one warm-up and one measured iteration of 200 ms, to check the setup)
/// and TARANTINO_BENCHMARK_SKIP_MONGO (true: without the mongo index and metadata structures).
class BenchmarkOptions {
public:
    using Environment = std::function<std::optional<std::string>(const std::string&)>;

    static BenchmarkOptions fromEnvironment();

    static BenchmarkOptions of(const Environment& environment);

    /// Two passes in a full run, the second in reverse order, so that whatever drifts during a run, such as the
    /// temperature or the writes left by the benchmark before, weighs alike on every structure and size; one in quick
    /// mode.
    std::vector<Pass> passes() const;

    Iterations iterations(const Iterations& declared) const;

    /// The sizes to run, in the order of the pass.
    std::vector<int> sizes(const std::vector<int>& declared, const Pass& pass) const;

    /// The structures to run, in the order of the pass; mongo only when it is not skipped.
    std::vector<std::string> structures(const std::vector<std::string>& declared, const Pass& pass) const;

    bool quick() const {
        return quick_;
    }

    bool skipMongo() const {
        return skipMongo_;
    }

private:
    bool quick_ = false;
    bool skipMongo_ = false;
    std::optional<std::vector<int>> sizes_;
};

}
