#include "crawler/tests/benchmarking/support/environment/BenchmarkPaths.hpp"

#include "tarantino/platform/Environment.hpp"
#include "tarantino/platform/Paths.hpp"

namespace tarantino::crawler::benchmarking {

namespace {

constexpr const char* SCRATCH_DIRECTORY = "tmp.noindex";

}

std::filesystem::path BenchmarkPaths::benchmarks() {
    return platform::pathOf(platform::environmentVariable("TARANTINO_BENCHMARKS", "benchmarks"));
}

std::filesystem::path BenchmarkPaths::workload() {
    return platform::pathOf(platform::environmentVariable("TARANTINO_WORKLOAD", "workload"));
}

std::filesystem::path BenchmarkPaths::scratch(const std::string& name) {
    return scratchRoot() / platform::pathOf(name);
}

std::filesystem::path BenchmarkPaths::scratchRoot() {
    return benchmarks() / SCRATCH_DIRECTORY;
}

std::string BenchmarkPaths::mongoUri(const std::string& database) {
    std::string server = platform::environmentVariable("TARANTINO_MONGO_URI", "mongodb://localhost:27017");
    while (!server.empty() && server.back() == '/') {
        server.pop_back();
    }
    return server + "/" + database;
}

}
