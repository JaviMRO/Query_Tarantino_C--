#include "query/tests/benchmarking/QueryWorkload.hpp"
#include "query/tests/benchmarking/WorkloadQuery.hpp"

#include <gtest/gtest.h>

#include <stdexcept>

namespace tarantino::query::benchmarking {
namespace {

TEST(WorkloadQueryTest, readsTheCategoryAndTheQueryOfALine) {
    EXPECT_EQ((WorkloadQuery{"long", "ship captain sea voyage"}), WorkloadQuery::parse("long: ship captain sea voyage"));
}

TEST(WorkloadQueryTest, rejectsALineWithoutCategory) {
    EXPECT_THROW(WorkloadQuery::parse("ship captain"), std::invalid_argument);
}

TEST(WorkloadQueryTest, selectsTheQueriesOfACategoryOrAllOfThem) {
    const std::vector<WorkloadQuery> workload{{"rare", "liliput"}, {"frequent", "love"}};

    EXPECT_EQ(std::vector<std::string>{"love"}, QueryWorkload::texts(workload, "frequent"));
    EXPECT_EQ((std::vector<std::string>{"liliput", "love"}), QueryWorkload::texts(workload, QueryWorkload::ALL_CATEGORIES));
}

}
}
