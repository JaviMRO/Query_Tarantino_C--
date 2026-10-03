#include "tarantino/crawler/CrawlerFactory.hpp"
#include "tarantino/crawler/adapters/datalake/time/TimeBasedDatalakeAdapter.hpp"
#include "tarantino/crawler/model/book/GutenbergText.hpp"
#include "tarantino/crawler/model/failure/DownloadException.hpp"
#include "tarantino/platform/Paths.hpp"
#include "tarantino/testing/TempDirectory.hpp"

#include "crawler/tests/conformance/ConformanceCases.hpp"

#include <gtest/gtest.h>

namespace tarantino::crawler::conformance {
namespace {

using model::BookText;
using model::DownloadException;
using model::StoredPaths;

std::string caseName(const ::testing::TestParamInfo<nlohmann::json>& info) {
    const nlohmann::json& testCase = info.param;
    if (testCase.contains("name")) {
        return ConformanceCases::testName(info.index, testCase.at("name").get<std::string>());
    }
    return ConformanceCases::testName(info.index, testCase.at("layout").get<std::string>() + " "
                                                          + std::to_string(testCase.at("id").get<int>()));
}

class SplitConformanceTest : public ::testing::TestWithParam<nlohmann::json> {};

TEST_P(SplitConformanceTest, splitsHeaderAndBody) {
    const nlohmann::json& testCase = GetParam();
    SCOPED_TRACE(testCase.at("name").get<std::string>());
    const std::string raw = testCase.at("raw").get<std::string>();
    if (testCase.contains("failure")) {
        try {
            model::GutenbergText::bookText(1, raw);
            FAIL() << "split without " << testCase.at("failure");
        } catch (const DownloadException& failure) {
            EXPECT_EQ(testCase.at("failure").get<std::string>(), model::name(failure.reason()));
        }
    } else {
        const BookText text = model::GutenbergText::bookText(1, raw);
        EXPECT_EQ(testCase.at("header").get<std::string>(), text.header);
        EXPECT_EQ(testCase.at("body").get<std::string>(), text.body);
    }
}

INSTANTIATE_TEST_SUITE_P(Spec, SplitConformanceTest, ::testing::ValuesIn(ConformanceCases::cases("split.json")), caseName);

class DatalakePathsConformanceTest : public ::testing::TestWithParam<nlohmann::json> {
protected:
    testing::TempDirectory root;
};

TEST_P(DatalakePathsConformanceTest, storesBooksAtTheirLayoutPaths) {
    const nlohmann::json& testCase = GetParam();
    const std::string layout = testCase.at("layout").get<std::string>();
    const int bookId = testCase.at("id").get<int>();
    SCOPED_TRACE(layout + " " + std::to_string(bookId) + " at " + testCase.at("saved_at").get<std::string>());
    const std::filesystem::path layoutRoot = root / (layout + "-" + std::to_string(bookId));
    const auto savedAt = platform::parseInstant(testCase.at("saved_at").get<std::string>());
    ASSERT_TRUE(savedAt.has_value());
    const std::shared_ptr<ports::DatalakeStorage> datalake =
            layout == "time" ? std::make_shared<adapters::TimeBasedDatalakeAdapter>(layoutRoot, platform::fixedClock(*savedAt))
                             : CrawlerFactory::datalake(CrawlerConfig{layoutRoot, layout, std::nullopt});

    const StoredPaths paths = datalake->save(BookText{bookId, "header", "body"});

    EXPECT_EQ(testCase.at("header").get<std::string>(), platform::portable(paths.header.lexically_relative(layoutRoot)));
    EXPECT_EQ(testCase.at("body").get<std::string>(), platform::portable(paths.body.lexically_relative(layoutRoot)));
}

INSTANTIATE_TEST_SUITE_P(Spec, DatalakePathsConformanceTest,
                         ::testing::ValuesIn(ConformanceCases::cases("datalake_paths.json")), caseName);

}
}
