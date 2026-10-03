// The query and metadata read benchmarks of SPEC §11, written to benchmarks/results/cpp-query.csv.

#include "query/tests/benchmarking/BenchmarkRunner.hpp"

#include "crawler/tests/benchmarking/support/environment/BenchmarkOptions.hpp"
#include "crawler/tests/benchmarking/support/environment/BenchmarkPaths.hpp"
#include "crawler/tests/benchmarking/support/files/FootprintLog.hpp"
#include "crawler/tests/benchmarking/support/files/ResultsFile.hpp"
#include "crawler/tests/benchmarking/support/harness/Harness.hpp"
#include "crawler/tests/benchmarking/support/results/Results.hpp"
#include "indexer/tests/benchmarking/support/PrebuiltIndexes.hpp"
#include "tarantino/platform/Environment.hpp"
#include "tarantino/platform/Paths.hpp"
#include "tarantino/query/QueryFactory.hpp"

#include <nlohmann/json.hpp>
#include <sqlite3.h>

#ifdef TARANTINO_WITH_MONGO
#include "indexer/tests/benchmarking/support/MongoStores.hpp"

#include <mongocxx/config/version.hpp>
#endif

#include <iostream>

namespace tarantino::query::benchmarking {

std::vector<std::string> indexStructures() {
    std::vector<std::string> structures{"json", "folders"};
    if (QueryFactory::withMongo()) {
        structures.emplace_back("mongo");
    }
    return structures;
}

std::vector<std::string> metadataBackends() {
    std::vector<std::string> backends{"sqlite"};
    if (QueryFactory::withMongo()) {
        backends.emplace_back("mongo");
    }
    return backends;
}

}

namespace {

using namespace tarantino::crawler::benchmarking;
using namespace tarantino::query::benchmarking;
using tarantino::indexer::benchmarking::PrebuiltIndexes;

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

/// query_time and query_time_p99: the mean and the 99th percentile of the times of each measured second (SPEC §11);
/// query_time_<category> comes from the benchmark itself through FootprintLog.
std::vector<ResultRow> queryTimeRows(const std::vector<Measurement>& measurements) {
    std::vector<ResultRow> rows;
    for (const Measurement& measurement : measurements) {
        if (measurement.method == "queryTime") {
            rows.push_back(Results::perIteration(measurement, "query_time", QUERY_TIME_UNIT, measurement.samples));
            rows.push_back(Results::perIteration(measurement, "query_time_p99", QUERY_TIME_UNIT, measurement.percentiles99));
        }
    }
    return rows;
}

}

int main(int argc, char** argv) {
    tarantino::platform::useUtf8Console();
    const bool runner = !Harness::isProcess(argc, argv);
    try {
        if (runner) {
            FootprintLog::drain(QUERY_SERVICE);
            PrebuiltIndexes::deleteAll();
            Harness::recordEnvironment(QUERY_SERVICE, libraries());
        }
        const auto measurements = Harness::run(QUERY_SERVICE,
                                               {IndexOpenBenchmark::definition(), MetadataQueryBenchmark::definition(),
                                                QueryTimeBenchmark::definition()},
                                               argc, argv);
        if (!measurements) {
            return Harness::processStatus();
        }
        const std::map<std::string, Metric> metrics{
                {"bookByIdTime", Metric::asMeasured("book_by_id_time", "\xC2\xB5s/op")},
                {"booksByAuthorTime", Metric::asMeasured("books_by_author_time", "\xC2\xB5s/op")},
                {"indexOpenTime", Metric::asMeasured("index_open_time", "ms")}};
        std::vector<ResultRow> rows = Results::rows(*measurements, metrics);
        for (const std::vector<ResultRow>& more : {queryTimeRows(*measurements), FootprintLog::drain(QUERY_SERVICE)}) {
            rows.insert(rows.end(), more.begin(), more.end());
        }
        std::cout << "Results written to " << tarantino::platform::portable(ResultsFile::write(QUERY_SERVICE, rows)) << std::endl;
        PrebuiltIndexes::deleteAll();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "query benchmarks: " << error.what() << std::endl;
        if (runner) {
            PrebuiltIndexes::deleteAll();
        }
        return 1;
    }
}
