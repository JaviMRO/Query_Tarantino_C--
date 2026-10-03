#pragma once

#include <chrono>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <string_view>

/// Instants and clocks as java.time has them, written and read in UTC with no time zone database.
namespace tarantino::platform {

/// An instant of the UTC time line with nanosecond precision, like java.time.Instant: file times compare at the full
/// precision the file system keeps (SPEC §6).
using Instant = std::chrono::time_point<std::chrono::system_clock, std::chrono::nanoseconds>;

/// Where the current instant comes from, like java.time.Clock: the system clock, or a fixed one in tests.
using Clock = std::function<Instant()>;

Instant now();

Clock systemClock();

Clock fixedClock(Instant instant);

/// The date and time of an instant in UTC (proleptic Gregorian calendar, as java.time).
struct UtcDateTime {
    std::int64_t year;
    unsigned month;
    unsigned day;
    unsigned hour;
    unsigned minute;
    unsigned second;
    std::int64_t nanosecond;
};

UtcDateTime utcDateTime(Instant instant);

/// The instant of a date and time in UTC; empty when a field is out of its range or the day does not exist.
std::optional<Instant> instantOf(const UtcDateTime& dateTime);

/// The day of the instant in UTC as yyyyMMdd, the first directory of the time layout (SPEC §6).
std::string utcDay(Instant instant);

/// The hour of the instant in UTC as HH, the second directory of the time layout (SPEC §6).
std::string utcHour(Instant instant);

/// Reads an instant written in ISO-8601 in UTC, such as 2025-09-25T14:03:00Z, as java.time.Instant.parse does: with an
/// optional fraction of second and Z or an offset such as +01:00.
std::optional<Instant> parseInstant(std::string_view text);

/// Writes the instant as java.time.Instant.toString does, such as 2025-09-25T14:03:00Z.
std::string formatInstant(Instant instant);

/// Reads an HTTP date as Java's DateTimeFormatter.RFC_1123_DATE_TIME does, such as "Thu, 01 Oct 2026 12:01:30 GMT":
/// the day of the week (checked against the date) and the seconds are optional, and the zone is GMT or an offset such
/// as +0200. Day and month names are read in any case.
std::optional<Instant> parseHttpDate(std::string_view text);

/// Writes the instant as an HTTP date, such as "Thu, 01 Oct 2026 12:01:30 GMT".
std::string formatHttpDate(Instant instant);

}
