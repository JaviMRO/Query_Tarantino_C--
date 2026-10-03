#include "query/tests/benchmarking/QueryWorkload.hpp"

#include "crawler/tests/benchmarking/support/environment/BenchmarkPaths.hpp"
#include "indexer/tests/benchmarking/support/BenchmarkStore.hpp"
#include "tarantino/platform/Files.hpp"
#include "tarantino/query/QueryConfig.hpp"
#include "tarantino/query/QueryFactory.hpp"
#include "tarantino/query/adapters/stopwords/FileStopwordsLoader.hpp"

#include <memory>

namespace tarantino::query::benchmarking {

using crawler::benchmarking::BenchmarkPaths;

namespace {

// Metadata is not read by the index benchmarks, so only the index is measured
class ConstantMetadata : public ports::MetadataReader {
public:
    std::optional<model::BookMetadata> book(int bookId) override {
        return model::BookMetadata{bookId, "title", "author", "English", "body.txt"};
    }

    std::vector<model::BookMetadata> booksBy(const std::string&) override {
        return {};
    }
};

}

commands::SearchCommand QueryWorkload::openSearch(const indexer::benchmarking::BenchmarkStore& store, const std::string& index,
                                                  const platform::StringSet& stopwords) {
    const QueryConfig config{store.datamarts(), index, "sqlite", store.mongoUri(), BenchmarkPaths::workload()};
    return commands::SearchCommand(QueryFactory::invertedIndex(config), std::make_shared<ConstantMetadata>(), stopwords);
}

platform::StringSet QueryWorkload::stopwords() {
    return adapters::FileStopwordsLoader(BenchmarkPaths::workload() / "stopwords.txt").stopwords();
}

std::vector<WorkloadQuery> QueryWorkload::queries() {
    std::vector<WorkloadQuery> queries;
    for (const std::string& line : platform::files::readLines(BenchmarkPaths::workload() / "queries.txt")) {
        if (!platform::strip(line).empty()) {
            queries.push_back(WorkloadQuery::parse(line));
        }
    }
    return queries;
}

std::vector<std::string> QueryWorkload::texts(const std::vector<WorkloadQuery>& queries, const std::string& category) {
    std::vector<std::string> texts;
    for (const WorkloadQuery& query : queries) {
        if (category == ALL_CATEGORIES || query.category == category) {
            texts.push_back(query.text);
        }
    }
    return texts;
}

}
