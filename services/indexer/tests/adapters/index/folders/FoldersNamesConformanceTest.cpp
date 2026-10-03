#include "indexer/src/adapters/index/folders/TermFiles.hpp"

#include "crawler/tests/conformance/ConformanceCases.hpp"
#include "tarantino/platform/Paths.hpp"

#include <gtest/gtest.h>

namespace tarantino::indexer::adapters {
namespace {

using crawler::conformance::ConformanceCases;

class FoldersNamesConformanceTest : public ::testing::TestWithParam<nlohmann::json> {};

TEST_P(FoldersNamesConformanceTest, namesTermFiles) {
    const nlohmann::json& testCase = GetParam();
    SCOPED_TRACE(testCase.at("name").get<std::string>());
    const std::filesystem::path root = "index";

    EXPECT_EQ(root / platform::pathOf(testCase.at("path").get<std::string>()),
              TermFiles::file(root, testCase.at("term").get<std::string>()));
}

INSTANTIATE_TEST_SUITE_P(Spec, FoldersNamesConformanceTest, ::testing::ValuesIn(ConformanceCases::cases("folders_names.json")),
                         [](const auto& info) { return ConformanceCases::testName(info.index, info.param.at("name").template get<std::string>()); });

}
}
