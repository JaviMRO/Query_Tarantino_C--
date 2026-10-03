// The datalake benchmarks of SPEC §11: write throughput, lookup, new books detection, recovery and footprint of each
// layout, written to benchmarks/results/cpp-crawler.csv.

#include "crawler/tests/benchmarking/BenchmarkRunner.hpp"

#include "crawler/tests/benchmarking/datalake/DatalakeBenchmarks.hpp"
#include "crawler/tests/benchmarking/support/datalake/DatalakeFixture.hpp"
#include "crawler/tests/benchmarking/support/datalake/RecoveryScenario.hpp"
#include "crawler/tests/benchmarking/support/environment/BenchmarkPaths.hpp"
#include "crawler/tests/benchmarking/support/files/Directories.hpp"
#include "crawler/tests/benchmarking/support/files/FootprintLog.hpp"
#include "crawler/tests/benchmarking/support/files/ResultsFile.hpp"
#include "crawler/tests/benchmarking/support/harness/Harness.hpp"
#include "crawler/tests/benchmarking/support/results/Results.hpp"
#include "crawler/tests/ports/FunctionDownloader.hpp"
#include "tarantino/platform/Environment.hpp"
#include "tarantino/platform/Paths.hpp"

#include <curl/curl.h>

#include <iostream>

namespace {

using namespace tarantino::crawler::benchmarking;

constexpr int RECOVERY_BOOKS = 100;

std::vector<ResultRow> recoveryRows() {
    BenchmarkDataset dataset = BenchmarkDataset::fromEnvironment();
    const std::vector<int> ids = dataset.ids(RECOVERY_BOOKS);
    std::vector<ResultRow> rows;
    for (const std::string& layout : DatalakeFixture::LAYOUTS) {
        const std::filesystem::path root = BenchmarkPaths::scratch("datalake-recovery-" + layout);
        Directories::remove(root);
        const auto downloader = tarantino::crawler::ports::FunctionDownloader::of([&dataset](int bookId) { return dataset.rawText(bookId); });
        const RecoveryOutcome outcome = RecoveryScenario(layout, root, downloader).run(ids);
        Directories::remove(root);
        rows.push_back(ResultRow::exact(layout, "recovery_ok", RECOVERY_BOOKS, outcome.recovered ? 1 : 0, "0 or 1"));
        rows.push_back(ResultRow::exact(layout, "recovery_leftover_files", RECOVERY_BOOKS, static_cast<double>(outcome.leftoverFiles), "files"));
    }
    return rows;
}

}

int main(int argc, char** argv) {
    tarantino::platform::useUtf8Console();
    try {
        if (!Harness::isProcess(argc, argv)) {
            FootprintLog::drain(CRAWLER_SERVICE);
            Harness::recordEnvironment(CRAWLER_SERVICE, {std::string("libcurl: ") + curl_version()});
        }
        const auto measurements = Harness::run(CRAWLER_SERVICE,
                                               {DatalakeLookupBenchmark::definition(), DatalakeWriteBenchmark::definition(),
                                                NewBooksDetectionBenchmark::definition()},
                                               argc, argv);
        if (!measurements) {
            return Harness::processStatus();
        }
        const std::map<std::string, Metric> metrics{
                {"writeThroughput", Metric::booksPerSecond("write_throughput")},
                {"lookupTime", Metric::asMeasured("lookup_time", "\xC2\xB5s/op")},
                {"newBooksDetectionTime", Metric::asMeasured("new_books_detection_time", "ms")}};
        std::vector<ResultRow> rows = Results::rows(*measurements, metrics);
        for (std::vector<ResultRow> more : {FootprintLog::drain(CRAWLER_SERVICE), recoveryRows()}) {
            rows.insert(rows.end(), more.begin(), more.end());
        }
        std::cout << "Results written to " << tarantino::platform::portable(ResultsFile::write(CRAWLER_SERVICE, rows)) << std::endl;
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "crawler benchmarks: " << error.what() << std::endl;
        return 1;
    }
}
