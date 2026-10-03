#include "crawler/tests/benchmarking/support/harness/Harness.hpp"

#include "crawler/tests/benchmarking/support/environment/BenchmarkPaths.hpp"
#include "crawler/tests/benchmarking/support/harness/Processes.hpp"
#include "crawler/tests/benchmarking/support/harness/Statistics.hpp"
#include "tarantino/platform/Environment.hpp"
#include "tarantino/platform/Files.hpp"
#include "tarantino/platform/Paths.hpp"
#include "tarantino/platform/Text.hpp"
#include "tarantino/platform/Unicode.hpp"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <limits>
#include <map>
#include <stdexcept>
#include <thread>

#if defined(_WIN32)
#include <windows.h>
#else
#include <sys/utsname.h>
#include <unistd.h>
#if defined(__APPLE__)
#include <sys/sysctl.h>
#endif
#endif

namespace tarantino::crawler::benchmarking {

namespace files = platform::files;

namespace {

constexpr const char* FORK = "--fork";
int forkStatus = 0;

using Clock = std::chrono::steady_clock;

struct IterationResult {
    double value;
    double percentile99;
};

double inUnit(Clock::duration duration, TimeUnit unit) {
    const double nanoseconds = std::chrono::duration<double, std::nano>(duration).count();
    return unit == TimeUnit::Milliseconds ? nanoseconds / 1e6 : nanoseconds / 1e3;
}

const char* unitName(TimeUnit unit) {
    return unit == TimeUnit::Milliseconds ? "ms/op" : "us/op";
}

std::string number(double value) {
    char text[64];
    std::snprintf(text, sizeof text, "%.17g", value);
    return text;
}

std::string shown(double value) {
    char text[64];
    std::snprintf(text, sizeof text, "%.3f", value);
    return text;
}

/// Ends an iteration after its time, as JMH's timer does: the calls see a flag, not the clock.
class IterationTimer {
public:
    explicit IterationTimer(std::chrono::milliseconds time)
        : thread_([this, time] {
              std::this_thread::sleep_for(time);
              done_.store(true, std::memory_order_release);
          }) {
    }

    IterationTimer(const IterationTimer&) = delete;
    IterationTimer& operator=(const IterationTimer&) = delete;

    ~IterationTimer() {
        thread_.join();
    }

    bool done() const {
        return done_.load(std::memory_order_acquire);
    }

private:
    std::atomic<bool> done_{false};
    std::thread thread_;
};

IterationResult iterate(const Benchmark& benchmark, const std::string& method, BenchmarkState& state,
                        const Iterations& iterations, bool measured, Blackhole& blackhole) {
    state.setupIteration(measured);
    IterationResult result{0.0, std::numeric_limits<double>::quiet_NaN()};
    if (benchmark.mode == Mode::SingleShotTime) {
        const auto start = Clock::now();
        state.invoke(method, blackhole);
        result.value = inUnit(Clock::now() - start, benchmark.unit);
    } else {
        std::uint64_t calls = 0;
        std::vector<Clock::duration> times;
        Clock::duration elapsed{};
        {
            const IterationTimer timer(iterations.time);
            const auto start = Clock::now();
            if (benchmark.mode == Mode::AverageTime) {
                do {
                    state.invoke(method, blackhole);
                    ++calls;
                } while (!timer.done());
            } else {
                // Every call is timed (SPEC §11: query_time)
                times.reserve(std::size_t{1} << 18);
                do {
                    const auto callStart = Clock::now();
                    state.invoke(method, blackhole);
                    times.push_back(Clock::now() - callStart);
                    ++calls;
                } while (!timer.done());
            }
            elapsed = Clock::now() - start;
        }
        if (benchmark.mode == Mode::AverageTime) {
            result.value = inUnit(elapsed, benchmark.unit) / static_cast<double>(calls);
        } else {
            Statistics statistics;
            for (const Clock::duration time : times) {
                statistics.add(inUnit(time, benchmark.unit));
            }
            result.value = statistics.mean();
            result.percentile99 = statistics.percentile(99);
        }
    }
    state.tearDownIteration(measured);
    return result;
}

const Benchmark& named(const std::vector<Benchmark>& benchmarks, const std::string& name) {
    const auto found = std::find_if(benchmarks.begin(), benchmarks.end(), [&](const Benchmark& benchmark) { return benchmark.name == name; });
    if (found == benchmarks.end()) {
        throw std::invalid_argument("Unknown benchmark: " + name);
    }
    return *found;
}

/// One process: the trial of a configuration, its warm-up and its measured iterations, written to the samples file.
int runProcess(const std::vector<Benchmark>& benchmarks, const std::vector<std::string>& arguments) {
    if (arguments.size() != 5) {
        throw std::invalid_argument("Expected: --fork <benchmark> <method> <structure> <books> <samples file>");
    }
    const Benchmark& benchmark = named(benchmarks, arguments[0]);
    const std::string& method = arguments[1];
    const int books = std::stoi(arguments[3]);
    const Iterations iterations = BenchmarkOptions::fromEnvironment().iterations(benchmark.iterations);
    Blackhole blackhole;
    std::unique_ptr<BenchmarkState> state = benchmark.state(arguments[2], books);
    for (int iteration = 1; iteration <= iterations.warmup; ++iteration) {
        const IterationResult result = iterate(benchmark, method, *state, iterations, false, blackhole);
        std::cout << "# Warmup Iteration " << iteration << ": " << shown(result.value) << " " << unitName(benchmark.unit) << std::endl;
    }
    std::string samples;
    for (int iteration = 1; iteration <= iterations.measurement; ++iteration) {
        const IterationResult result = iterate(benchmark, method, *state, iterations, true, blackhole);
        std::cout << "Iteration " << iteration << ": " << shown(result.value) << " " << unitName(benchmark.unit) << std::endl;
        samples += std::to_string(iteration) + "," + number(result.value) + ","
                   + (std::isnan(result.percentile99) ? "" : number(result.percentile99)) + "\n";
    }
    state->tearDownTrial();
    state.reset();
    files::writeString(platform::pathOf(arguments[4]), samples);
    return 0;
}

std::string key(const std::string& benchmark, const std::string& method, const std::string& structure, int books) {
    return benchmark + "." + method + "," + structure + "," + std::to_string(books);
}

std::string operatingSystem() {
#if defined(_WIN32)
    using RtlGetVersionFunction = LONG(WINAPI*)(PRTL_OSVERSIONINFOW);
    RTL_OSVERSIONINFOW version{};
    version.dwOSVersionInfoSize = sizeof version;
    if (const HMODULE ntdll = GetModuleHandleW(L"ntdll.dll")) {
        if (const auto rtlGetVersion = reinterpret_cast<RtlGetVersionFunction>(reinterpret_cast<void*>(GetProcAddress(ntdll, "RtlGetVersion")))) {
            rtlGetVersion(&version);
        }
    }
    return "Windows " + std::to_string(version.dwMajorVersion) + "." + std::to_string(version.dwMinorVersion) + " build "
           + std::to_string(version.dwBuildNumber);
#else
    utsname name{};
    uname(&name);
    return std::string(name.sysname) + " " + name.release + " " + name.machine;
#endif
}

std::string processor() {
#if defined(_WIN32)
    wchar_t model[256];
    DWORD size = sizeof model;
    if (RegGetValueW(HKEY_LOCAL_MACHINE, L"HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0", L"ProcessorNameString",
                     RRF_RT_REG_SZ, nullptr, model, &size) == ERROR_SUCCESS) {
        return platform::portable(std::filesystem::path(model));
    }
    return "unknown";
#elif defined(__APPLE__)
    char model[256];
    std::size_t size = sizeof model;
    return sysctlbyname("machdep.cpu.brand_string", model, &size, nullptr, 0) == 0 ? std::string(model) : "Apple silicon";
#else
    std::ifstream cpuinfo("/proc/cpuinfo");
    for (std::string line; std::getline(cpuinfo, line);) {
        if (line.starts_with("model name")) {
            return std::string(platform::strip(line.substr(line.find(':') + 1)));
        }
    }
    return "unknown";
#endif
}

double memoryGigabytes() {
#if defined(_WIN32)
    MEMORYSTATUSEX status{};
    status.dwLength = sizeof status;
    GlobalMemoryStatusEx(&status);
    return static_cast<double>(status.ullTotalPhys) / (1 << 30);
#else
    return static_cast<double>(sysconf(_SC_PHYS_PAGES)) * static_cast<double>(sysconf(_SC_PAGE_SIZE)) / (1 << 30);
#endif
}

std::string compiler() {
#if defined(__clang__)
    return std::string("Clang ") + __clang_version__;
#elif defined(__GNUC__)
    return std::string("GCC ") + __VERSION__;
#elif defined(_MSC_VER)
    return "MSVC " + std::to_string(_MSC_FULL_VER);
#else
    return "unknown";
#endif
}

}

std::optional<std::vector<Measurement>> Harness::run(const std::string& service, const std::vector<Benchmark>& benchmarks,
                                                     int argc, char** argv) {
    const std::vector<std::string> arguments = platform::commandLineArguments(argc, argv);
    if (!arguments.empty() && arguments.front() == FORK) {
        try {
            forkStatus = runProcess(benchmarks, std::vector<std::string>(arguments.begin() + 1, arguments.end()));
        } catch (const std::exception& error) {
            std::cerr << "The benchmark process failed: " << error.what() << std::endl;
            forkStatus = 1;
        }
        return std::nullopt;
    }
    const BenchmarkOptions options = BenchmarkOptions::fromEnvironment();
    std::vector<const Benchmark*> ordered;
    for (const Benchmark& benchmark : benchmarks) {
        ordered.push_back(&benchmark);
    }
    std::sort(ordered.begin(), ordered.end(), [](const Benchmark* first, const Benchmark* second) { return first->name < second->name; });
    const std::filesystem::path self = Processes::currentExecutable();
    const std::filesystem::path samplesFile = BenchmarkPaths::scratch("samples-" + service + ".csv");
    std::map<std::string, Measurement> measurements;
    std::vector<std::string> order;
    const std::vector<Pass> passes = options.passes();
    for (const Pass& pass : passes) {
        std::string raw = "benchmark,method,structure,n_books,iteration,value,p99,unit\n";
        for (const Benchmark* benchmark : ordered) {
            std::vector<std::string> methods = benchmark->methods;
            std::sort(methods.begin(), methods.end());
            for (const std::string& method : methods) {
                for (const std::string& structure : options.structures(benchmark->structures, pass)) {
                    for (const int books : options.sizes(benchmark->books, pass)) {
                        std::cout << "# Pass " << pass.number << "/" << passes.size() << ": " << benchmark->name << "." << method
                                  << " (" << benchmark->structureParameter << " = " << structure << ", books = " << books << ")"
                                  << std::endl;
                        files::createDirectories(samplesFile.parent_path());
                        files::removeIfExists(samplesFile);
                        const int exitStatus = Processes::run(self, {FORK, benchmark->name, method, structure, std::to_string(books),
                                                                     platform::portable(samplesFile)});
                        if (exitStatus != 0) {
                            throw std::runtime_error("The process of " + benchmark->name + "." + method + " (" + structure + ", "
                                                     + std::to_string(books) + " books) failed with status "
                                                     + std::to_string(exitStatus) + "; no results are written");
                        }
                        const std::string configuration = key(benchmark->name, method, structure, books);
                        auto [entry, inserted] = measurements.try_emplace(configuration, Measurement{benchmark->name, method, structure, books, {}, {}});
                        if (inserted) {
                            order.push_back(configuration);
                        }
                        for (const std::string& line : files::readLines(samplesFile)) {
                            const std::size_t first = line.find(',');
                            const std::size_t second = line.find(',', first + 1);
                            const std::string value = line.substr(first + 1, second - first - 1);
                            const std::string percentile = line.substr(second + 1);
                            entry->second.samples.push_back(std::stod(value));
                            if (!percentile.empty()) {
                                entry->second.percentiles99.push_back(std::stod(percentile));
                            }
                            raw += benchmark->name + "," + method + "," + structure + "," + std::to_string(books) + "," + line
                                   + "," + unitName(benchmark->unit) + "\n";
                        }
                    }
                }
            }
        }
        const std::filesystem::path rawFile = BenchmarkPaths::benchmarks() / service / ("cpp-results-pass-" + std::to_string(pass.number) + ".csv");
        files::createDirectories(rawFile.parent_path());
        files::writeString(rawFile, raw);
    }
    files::removeIfExists(samplesFile);
    std::vector<Measurement> ordered_measurements;
    for (const std::string& configuration : order) {
        ordered_measurements.push_back(std::move(measurements.at(configuration)));
    }
    return ordered_measurements;
}

int Harness::processStatus() {
    return forkStatus;
}

bool Harness::isProcess(int argc, char** argv) {
    const std::vector<std::string> arguments = platform::commandLineArguments(argc, argv);
    return !arguments.empty() && arguments.front() == FORK;
}

void Harness::recordEnvironment(const std::string& service, const std::vector<std::string>& libraries) {
    std::string text = "language: cpp\n";
    text += "service: " + service + "\n";
    text += "operating system: " + operatingSystem() + "\n";
    text += "processor: " + processor() + " (" + std::to_string(std::thread::hardware_concurrency()) + " threads)\n";
    char memory[32];
    std::snprintf(memory, sizeof memory, "%.1f", memoryGigabytes());
    text += std::string("memory: ") + memory + " GB\n";
    text += "compiler: " + compiler() + ", C++ " + std::to_string(__cplusplus) + "\n";
#ifdef NDEBUG
    text += "build: release\n";
#else
    text += "build: debug (measurements are not comparable)\n";
#endif
    text += "unicode: " + platform::unicode::unicodeVersion() + " (ICU " + platform::unicode::icuVersion() + ")\n";
    for (const std::string& library : libraries) {
        text += library + "\n";
    }
    const std::filesystem::path file = BenchmarkPaths::benchmarks() / service / "cpp-environment.txt";
    files::createDirectories(file.parent_path());
    files::writeString(file, text);
    std::cout << text;
}

}
