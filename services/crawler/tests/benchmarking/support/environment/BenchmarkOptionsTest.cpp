#include "crawler/tests/benchmarking/support/environment/BenchmarkOptions.hpp"

#include <gtest/gtest.h>

#include <map>

namespace tarantino::crawler::benchmarking {
namespace {

BenchmarkOptions options(std::map<std::string, std::string> variables) {
    return BenchmarkOptions::of([variables](const std::string& name) -> std::optional<std::string> {
        const auto found = variables.find(name);
        return found == variables.end() ? std::nullopt : std::optional(found->second);
    });
}

const std::vector<std::string> LAYOUTS{"time", "book", "batch"};
const std::vector<int> SIZES{100, 300, 1000};

TEST(BenchmarkOptionsTest, runsASecondPassWithEveryParameterInReverseOrder) {
    const BenchmarkOptions full = options({});
    const std::vector<Pass> passes = full.passes();

    ASSERT_EQ(2u, passes.size());
    EXPECT_EQ(LAYOUTS, full.structures(LAYOUTS, passes[0]));
    EXPECT_EQ((std::vector<std::string>{"batch", "book", "time"}), full.structures(LAYOUTS, passes[1]));
    EXPECT_EQ((std::vector<int>{1000, 300, 100}), full.sizes(SIZES, passes[1]));
}

TEST(BenchmarkOptionsTest, reversesOverriddenSizesWithoutRepeatingThem) {
    const BenchmarkOptions overridden = options({{"TARANTINO_BENCHMARK_BOOKS", "100,300"}});

    EXPECT_EQ((std::vector<int>{100, 300}), overridden.sizes(SIZES, overridden.passes()[0]));
    EXPECT_EQ((std::vector<int>{300, 100}), overridden.sizes(SIZES, overridden.passes()[1]));
}

TEST(BenchmarkOptionsTest, runsOnePassOfOneShortIterationInQuickMode) {
    const BenchmarkOptions quick = options({{"TARANTINO_BENCHMARK_QUICK", "true"}});

    EXPECT_EQ(1u, quick.passes().size());
    const Iterations iterations = quick.iterations(Iterations{3, 5, std::chrono::seconds(1)});
    EXPECT_EQ(1, iterations.warmup);
    EXPECT_EQ(1, iterations.measurement);
    EXPECT_EQ(std::chrono::milliseconds(200), iterations.time);
}

TEST(BenchmarkOptionsTest, leavesMongoOutWhenAsked) {
    const BenchmarkOptions withoutMongo = options({{"TARANTINO_BENCHMARK_SKIP_MONGO", "TRUE"}});

    EXPECT_EQ((std::vector<std::string>{"json", "folders"}), withoutMongo.structures({"json", "folders", "mongo"}, withoutMongo.passes()[0]));
}

TEST(BenchmarkOptionsTest, rejectsSizesThatAreNotPositiveIntegers) {
    EXPECT_THROW(options({{"TARANTINO_BENCHMARK_BOOKS", "100,many"}}), std::invalid_argument);
}

}
}
