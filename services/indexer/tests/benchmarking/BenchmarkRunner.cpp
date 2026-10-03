// The index and metadata insertion benchmarks of SPEC §11, written to benchmarks/results/cpp-indexer.csv.

#include "indexer/tests/benchmarking/BenchmarkRunner.hpp"

#include "crawler/tests/benchmarking/support/environment/BenchmarkOptions.hpp"
#include "crawler/tests/benchmarking/support/environment/BenchmarkPaths.hpp"
#include "crawler/tests/benchmarking/support/files/FootprintLog.hpp"
#include "crawler/tests/benchmarking/support/files/ResultsFile.hpp"
#include "crawler/tests/benchmarking/support/harness/Harness.hpp"
#include "crawler/tests/benchmarking/support/results/Results.hpp"
#include "indexer/tests/benchmarking/support/PrebuiltIndexes.hpp"
#include "tarantino/indexer/IndexerFactory.hpp"
#include "tarantino/platform/Environment.hpp"
#include "tarantino/platform/Paths.hpp"

#include <nlohmann/json.hpp>
#include <sqlite3.h>

#ifdef TARANTINO_WITH_MONGO
#include "indexer/tests/benchmarking/support/MongoStores.hpp"

#include <mongocxx/config/version.hpp>
#endif

#include <iostream>

namespace tarantino::indexer::benchmarking {

std::vector<std::string> indexStructures() {
    std::vector<std::string> structures{"json", "folders"};
    if (IndexerFactory::withMongo()) {
        structures.emplace_back("mongo");
    }
    return structures;
}

std::vector<std::string> metadataBackends() {
    std::vector<std::string> backends{"sqlite"};
    if (IndexerFactory::withMongo()) {
        backends.emplace_back("mongo");
    }
    return backends;
}

}

namespace {

using namespace tarantino::crawler::benchmarking;
using namespace tarantino::indexer::benchmarking;

std::vector<std::string> libraries() {
    std::vector<std::string> versions{std::string("sqlite: ") + sqlite3_libversion(),
                                      "nlohmann/json: " + std::to_string(NLOHMANN_JSON_VERSION_MAJOR) + "." + std::to_string(NLOHMANN_JSON_VERSION_MINOR)
                                              + "." + std::to_string(NLOHMANN_JSON_VERSION_PATCH)};
#ifdef TARANTINO_WITH_MONGO
    versions.push_back(std::string("mongocxx: ") + MONGOCXX_VERSION_STRING);
    versions.push_back("MongoDB server: " + (BenchmarkOptions::fromEnvironment().skipMongo()
                                                     ? std::string("not used (TARANTINO_BENCHMARK_SKIP_MONGO)")
                                                     : tarantino::indexer::benchmarking::MongoStores::serverVersion(BenchmarkPaths::mongoUri("tarantino"))));
#else
    versions.emplace_back("mongocxx: not in this build, the mongo structures are left out");
#endif
    return versions;
}

}

int main(int argc, char** argv) {
    tarantino::platform::useUtf8Console();
    const bool runner = !Harness::isProcess(argc, argv);
    try {
        if (runner) {
            FootprintLog::drain(INDEXER_SERVICE);
            PrebuiltIndexes::deleteAll();
            Harness::recordEnvironment(INDEXER_SERVICE, libraries());
        }
        const auto measurements = Harness::run(INDEXER_SERVICE,
                                               {FullIndexBuildBenchmark::definition(), IncrementalUpdateBenchmark::definition(),
                                                MetadataInsertionBenchmark::definition()},
                                               argc, argv);
        if (!measurements) {
            return Harness::processStatus();
        }
        const std::map<std::string, Metric> metrics{
                {"fullBuildTime", Metric::asMeasured("full_build_time", "ms")},
                {"incrementalUpdateTime", Metric::perBook("incremental_update_time", IncrementalUpdateBenchmark::BOOKS_FLUSHED_ONE_BY_ONE)},
                {"batchUpdateTime", Metric::perBook("batch_update_time", IncrementalUpdateBenchmark::BOOKS_FLUSHED_TOGETHER)},
                {"bulkInsertionTime", Metric::asMeasured("bulk_insertion_time", "ms")}};
        std::vector<ResultRow> rows = Results::rows(*measurements, metrics);
        const std::vector<ResultRow> footprint = FootprintLog::drain(INDEXER_SERVICE);
        rows.insert(rows.end(), footprint.begin(), footprint.end());
        std::cout << "Results written to " << tarantino::platform::portable(ResultsFile::write(INDEXER_SERVICE, rows)) << std::endl;
        PrebuiltIndexes::deleteAll();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "indexer benchmarks: " << error.what() << std::endl;
        if (runner) {
            PrebuiltIndexes::deleteAll();
        }
        return 1;
    }
}
