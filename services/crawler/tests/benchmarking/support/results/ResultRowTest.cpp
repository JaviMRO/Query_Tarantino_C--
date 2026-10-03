#include "crawler/tests/benchmarking/support/results/ResultRow.hpp"

#include <gtest/gtest.h>

#include <cmath>

namespace tarantino::crawler::benchmarking {
namespace {

TEST(ResultRowTest, writesTheSpecCsvLineWithThreeDecimals) {
    EXPECT_EQ("cpp,time,write_throughput,1000,812.400,35.250,books/s",
              (ResultRow{"time", "write_throughput", 1000, 812.4, 35.25, "books/s"}).csvLine());
}

TEST(ResultRowTest, writesAnUnknownErrorAsAnEmptyField) {
    EXPECT_EQ("cpp,json,query_time,100,2.958,,\xC2\xB5s/query", ResultRow::sample("json", "query_time", 100, 2.958, "\xC2\xB5s/query").csvLine());
}

TEST(ResultRowTest, exactRowsHaveNoError) {
    EXPECT_EQ(0.0, ResultRow::exact("book", "file_count", 100, 200, "files").error);
}

TEST(ResultRowTest, readsBackTheLinesItWrites) {
    const ResultRow row{"mongo", "disk_usage", 500, 34'700'000, 0, "bytes"};

    EXPECT_EQ(row, ResultRow::parse(row.csvLine()));
    EXPECT_TRUE(std::isnan(ResultRow::parse("cpp,json,query_time,100,2.958,,\xC2\xB5s/query").error));
}

}
}
