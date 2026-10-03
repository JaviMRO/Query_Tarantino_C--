#include "tarantino/indexer/IndexerFactory.hpp"
#include "tarantino/indexer/model/book/HeaderParser.hpp"
#include "tarantino/indexer/model/terms/Tokenizer.hpp"
#include "tarantino/platform/Files.hpp"
#include "tarantino/platform/Paths.hpp"
#include "tarantino/testing/TempDirectory.hpp"

#include "crawler/tests/conformance/ConformanceCases.hpp"

#include <gtest/gtest.h>

namespace tarantino::indexer::conformance {
namespace {

namespace files = platform::files;
using crawler::conformance::ConformanceCases;
using model::BookText;

std::string named(const ::testing::TestParamInfo<nlohmann::json>& info) {
    const nlohmann::json& testCase = info.param;
    if (testCase.contains("name")) {
        return ConformanceCases::testName(info.index, testCase.at("name").get<std::string>());
    }
    return ConformanceCases::testName(info.index, testCase.at("layout").get<std::string>() + " "
                                                          + std::to_string(testCase.at("id").get<int>()));
}

class DatalakePathsConformanceTest : public ::testing::TestWithParam<nlohmann::json> {
protected:
    testing::TempDirectory root;
};

TEST_P(DatalakePathsConformanceTest, readsBooksAtTheirLayoutPaths) {
    const nlohmann::json& testCase = GetParam();
    const std::string layout = testCase.at("layout").get<std::string>();
    const int bookId = testCase.at("id").get<int>();
    SCOPED_TRACE(layout + " " + std::to_string(bookId) + " at " + testCase.at("saved_at").get<std::string>());
    const std::filesystem::path layoutRoot = root / (layout + "-" + std::to_string(bookId));
    const std::filesystem::path header = layoutRoot / platform::pathOf(testCase.at("header").get<std::string>());
    const std::filesystem::path body = layoutRoot / platform::pathOf(testCase.at("body").get<std::string>());
    files::createDirectories(header.parent_path());
    files::writeString(header, "header of " + std::to_string(bookId));
    files::writeString(body, "body of " + std::to_string(bookId));
    const IndexerConfig config{layoutRoot, layout, "datamarts", "json", "sqlite", "mongodb://localhost:27017", "workload"};

    EXPECT_EQ((BookText{bookId, "header of " + std::to_string(bookId), "body of " + std::to_string(bookId), body}),
              IndexerFactory::datalakeReader(config)->bookText(bookId));
}

INSTANTIATE_TEST_SUITE_P(Spec, DatalakePathsConformanceTest,
                         ::testing::ValuesIn(ConformanceCases::cases("datalake_paths.json")), named);

class HeaderFieldsConformanceTest : public ::testing::TestWithParam<nlohmann::json> {};

TEST_P(HeaderFieldsConformanceTest, extractsHeaderFields) {
    const nlohmann::json& testCase = GetParam();
    SCOPED_TRACE(testCase.at("name").get<std::string>());

    const model::Book book = model::HeaderParser().book(BookText{1, testCase.at("header").get<std::string>(), "", "1.body.txt"});

    EXPECT_EQ(ConformanceCases::textOrNull(testCase.at("title")), book.title);
    EXPECT_EQ(ConformanceCases::textOrNull(testCase.at("author")), book.author);
    EXPECT_EQ(ConformanceCases::textOrNull(testCase.at("language")), book.language);
}

INSTANTIATE_TEST_SUITE_P(Spec, HeaderFieldsConformanceTest,
                         ::testing::ValuesIn(ConformanceCases::cases("header_fields.json")), named);

class TermsConformanceTest : public ::testing::TestWithParam<nlohmann::json> {};

TEST_P(TermsConformanceTest, countsTermsOfABody) {
    const nlohmann::json& testCase = GetParam();
    SCOPED_TRACE(testCase.at("name").get<std::string>());
    const auto stopwords = ConformanceCases::stopwords(ConformanceCases::file("terms.json"));
    const model::Tokenizer tokenizer(platform::StringSet(stopwords.begin(), stopwords.end()));
    platform::StringMap<int> expected;
    for (const auto& [term, frequency] : testCase.at("terms").items()) {
        expected.emplace(term, frequency.get<int>());
    }

    EXPECT_EQ(expected, tokenizer.occurrences(1, testCase.at("text").get<std::string>()).frequencies);
}

INSTANTIATE_TEST_SUITE_P(Spec, TermsConformanceTest, ::testing::ValuesIn(ConformanceCases::cases("terms.json")), named);

}
}
