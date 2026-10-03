#include "tarantino/platform/Time.hpp"

#include <gtest/gtest.h>

namespace tarantino::platform {
namespace {

Instant instant(std::string_view text) {
    const auto parsed = parseInstant(text);
    EXPECT_TRUE(parsed.has_value()) << text;
    return parsed.value_or(Instant{});
}

TEST(TimeTest, namesTheDayAndHourDirectoriesInUtc) {
    EXPECT_EQ("20250925", utcDay(instant("2025-09-25T14:03:00Z")));
    EXPECT_EQ("14", utcHour(instant("2025-09-25T14:03:00Z")));
    EXPECT_EQ("20250105", utcDay(instant("2025-01-05T03:59:59Z")));
    EXPECT_EQ("03", utcHour(instant("2025-01-05T03:59:59Z")));
    EXPECT_EQ("20240229", utcDay(instant("2024-02-29T23:59:59Z")));
}

TEST(TimeTest, readsAndWritesInstantsAsJavaTime) {
    EXPECT_EQ("2025-09-25T14:03:00Z", formatInstant(instant("2025-09-25T14:03:00Z")));
    EXPECT_EQ("2025-09-25T15:29:00.123456700Z", formatInstant(instant("2025-09-25T15:29:00.1234567Z")));
    EXPECT_EQ("1969-12-31T23:59:59.500Z", formatInstant(instant("1969-12-31T23:59:59.5Z")));
    EXPECT_EQ(instant("2025-09-25T13:03:00Z"), instant("2025-09-25T14:03:00+01:00"));
    EXPECT_EQ(Instant{}, instant("1970-01-01T00:00:00Z"));
}

TEST(TimeTest, rejectsInstantsThatAreNotIso8601OrDoNotExist) {
    EXPECT_EQ(std::nullopt, parseInstant("2025-09-25 14:03:00Z"));
    EXPECT_EQ(std::nullopt, parseInstant("2025-02-29T00:00:00Z"));
    EXPECT_EQ(std::nullopt, parseInstant("2025-09-25T24:00:00Z"));
    EXPECT_EQ(std::nullopt, parseInstant("2025-09-25T14:03:00"));
}

TEST(TimeTest, readsHttpDatesAsRfc1123) {
    EXPECT_EQ(instant("2026-10-01T12:01:30Z"), parseHttpDate("Thu, 01 Oct 2026 12:01:30 GMT"));
    EXPECT_EQ(instant("2026-10-01T12:01:00Z"), parseHttpDate("1 Oct 2026 12:01 GMT"));
    EXPECT_EQ(instant("2026-10-01T10:01:30Z"), parseHttpDate("Thu, 01 Oct 2026 12:01:30 +0200"));
    EXPECT_EQ(instant("2026-10-01T12:01:30Z"), parseHttpDate("thu, 01 oct 2026 12:01:30 gmt"));
}

TEST(TimeTest, rejectsHttpDatesWithAWrongDayOfTheWeekOrAnotherFormat) {
    EXPECT_EQ(std::nullopt, parseHttpDate("Fri, 01 Oct 2026 12:01:30 GMT"));
    EXPECT_EQ(std::nullopt, parseHttpDate("Thursday, 01-Oct-26 12:01:30 GMT"));
    EXPECT_EQ(std::nullopt, parseHttpDate("120"));
}

TEST(TimeTest, writesHttpDates) {
    EXPECT_EQ("Thu, 01 Oct 2026 12:01:30 GMT", formatHttpDate(instant("2026-10-01T12:01:30Z")));
}

TEST(TimeTest, givesFixedClocksForTests) {
    const Clock clock = fixedClock(instant("2025-09-25T14:30:00Z"));
    EXPECT_EQ(instant("2025-09-25T14:30:00Z"), clock());
    EXPECT_GT(systemClock()(), instant("2025-01-01T00:00:00Z"));
}

}
}
