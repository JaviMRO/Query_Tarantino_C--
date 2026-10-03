#pragma once

#include "crawler/tests/benchmarking/support/environment/BenchmarkOptions.hpp"

#include <concepts>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace tarantino::crawler::benchmarking {

/// How a benchmark is timed, as JMH's modes (SPEC §11).
enum class Mode {
    /// Each iteration times one call: a sample is the time of one run.
    SingleShotTime,
    /// Each iteration repeats the call for its time: a sample is that time divided by the calls completed.
    AverageTime,
    /// As AverageTime, timing every call too: a sample is the mean of the calls of an iteration, and its 99th percentile.
    SampleTime
};

enum class TimeUnit {
    Milliseconds,
    Microseconds
};

/// Keeps the results of the measured calls observable, as JMH's Blackhole, so that the compiler cannot drop the work.
class Blackhole {
public:
    /// Any integer or bool. One overload per type would make some calls ambiguous on macOS, where std::size_t
    /// (unsigned long) is not std::uint64_t (unsigned long long), as it is on Linux and Windows.
    template <std::integral Value>
    void consume(Value value) {
        sink_ = sink_ + static_cast<std::uint64_t>(value);
    }

private:
    volatile std::uint64_t sink_ = 0;
};

/// The state of one benchmark process, as a JMH @State class: its constructor is the trial setup, its destructor
/// follows tearDownTrial, and invoke is a @Benchmark method.
class BenchmarkState {
public:
    virtual ~BenchmarkState() = default;

    virtual void setupIteration(bool /*measured*/) {
    }

    virtual void tearDownIteration(bool /*measured*/) {
    }

    virtual void tearDownTrial() {
    }

    virtual void invoke(const std::string& method, Blackhole& blackhole) = 0;
};

/// A benchmark class: its methods, how they are timed and the parameters of its processes (SPEC §11).
struct Benchmark {
    std::string name;
    std::vector<std::string> methods;
    Mode mode;
    TimeUnit unit;
    Iterations iterations;
    /// layout, index or metadata, and the structures it compares.
    std::string structureParameter;
    std::vector<std::string> structures;
    std::vector<int> books{100, 300, 1000};
    /// The state of a process for a structure and a number of books.
    std::function<std::unique_ptr<BenchmarkState>(const std::string& structure, int books)> state;
};

}
