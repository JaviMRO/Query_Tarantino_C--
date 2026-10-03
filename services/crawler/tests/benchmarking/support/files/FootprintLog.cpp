#include "crawler/tests/benchmarking/support/files/FootprintLog.hpp"

#include "crawler/tests/benchmarking/support/environment/BenchmarkPaths.hpp"
#include "crawler/tests/benchmarking/support/harness/Statistics.hpp"
#include "tarantino/platform/Files.hpp"

#include <map>

namespace tarantino::crawler::benchmarking {

namespace files = platform::files;

namespace {

std::filesystem::path exactFile(const std::string& service) {
    return BenchmarkPaths::scratch("footprint-" + service + ".csv");
}

std::filesystem::path samplesFile(const std::string& service) {
    return BenchmarkPaths::scratch("footprint-samples-" + service + ".csv");
}

std::vector<ResultRow> rows(const std::filesystem::path& file) {
    std::vector<ResultRow> rows;
    if (!files::exists(file)) {
        return rows;
    }
    for (const std::string& line : files::readLines(file)) {
        if (!line.empty()) {
            rows.push_back(ResultRow::parse(line));
        }
    }
    return rows;
}

std::string measure(const ResultRow& row) {
    return row.structure + "," + row.metric + "," + std::to_string(row.books);
}

}

void FootprintLog::append(const std::string& service, const std::vector<ResultRow>& rows) {
    appendTo(exactFile(service), rows);
}

void FootprintLog::appendSample(const std::string& service, const ResultRow& sample) {
    appendTo(samplesFile(service), std::vector<ResultRow>{sample});
}

std::vector<ResultRow> FootprintLog::drain(const std::string& service) {
    return drain(exactFile(service), samplesFile(service));
}

std::vector<ResultRow> FootprintLog::drain(const std::filesystem::path& exact, const std::filesystem::path& samples) {
    std::vector<ResultRow> drained;
    std::vector<std::string> order;
    std::map<std::string, ResultRow> firstPerMeasure;
    for (const ResultRow& row : rows(exact)) {
        if (firstPerMeasure.emplace(measure(row), row).second) {
            order.push_back(measure(row));
        }
    }
    for (const std::string& key : order) {
        drained.push_back(firstPerMeasure.at(key));
    }
    std::vector<std::string> sampleOrder;
    std::map<std::string, std::vector<ResultRow>> samplesPerMeasure;
    for (const ResultRow& row : rows(samples)) {
        auto& measured = samplesPerMeasure[measure(row)];
        if (measured.empty()) {
            sampleOrder.push_back(measure(row));
        }
        measured.push_back(row);
    }
    for (const std::string& key : sampleOrder) {
        const std::vector<ResultRow>& measured = samplesPerMeasure.at(key);
        Statistics statistics;
        for (const ResultRow& sample : measured) {
            statistics.add(sample.value);
        }
        const ResultRow& first = measured.front();
        drained.push_back(ResultRow{first.structure, first.metric, first.books, statistics.mean(),
                                    statistics.meanErrorAt(ResultRow::CONFIDENCE), first.unit});
    }
    files::removeIfExists(exact);
    files::removeIfExists(samples);
    return drained;
}

void FootprintLog::appendTo(const std::filesystem::path& file, const std::vector<ResultRow>& rows) {
    files::createDirectories(file.parent_path());
    std::string lines;
    for (const ResultRow& row : rows) {
        lines += row.csvLine() + "\n";
    }
    files::append(file, lines);
}

}
