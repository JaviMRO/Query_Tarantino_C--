#include "tarantino/control/model/StepReport.hpp"

#include <gtest/gtest.h>

namespace tarantino::control::model {
namespace {

TEST(StepReportTest, describesTheActionBookAndOutcome) {
    const StepReport report{NextStep::download(1342), Outcome::success("stored in datalake/1342")};

    EXPECT_EQ("DOWNLOAD 1342: stored in datalake/1342", report.description());
    EXPECT_FALSE(report.idle());
}

TEST(StepReportTest, summarizesTheBooksOfABatch) {
    const StepReport report{NextStep::index({84, 1342, 2701}), Outcome::success("3 indexed")};

    EXPECT_EQ("INDEX 3 books (84…2701): 3 indexed", report.description());
}

TEST(StepReportTest, isIdleOnlyForIdleSteps) {
    EXPECT_TRUE((StepReport{NextStep::idle(), Outcome::success("nothing left to do")}).idle());
}

}
}
