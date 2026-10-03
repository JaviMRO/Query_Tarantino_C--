#include "tarantino/platform/Time.hpp"

#include <array>
#include <cctype>
#include <cstdio>

namespace tarantino::platform {

namespace {

constexpr std::int64_t NANOS_PER_SECOND = 1'000'000'000;
constexpr std::int64_t SECONDS_PER_DAY = 86'400;
// Instants are nanoseconds since the epoch in 64 bits: years 1678 to 2262
constexpr std::int64_t FIRST_YEAR = 1678;
constexpr std::int64_t LAST_YEAR = 2261;
constexpr std::array<std::string_view, 7> WEEKDAYS{"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
constexpr std::array<std::string_view, 12> MONTHS{"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                                                  "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

std::int64_t floorDivide(std::int64_t dividend, std::int64_t divisor) {
    std::int64_t quotient = dividend / divisor;
    if (dividend % divisor != 0 && ((dividend < 0) != (divisor < 0))) {
        --quotient;
    }
    return quotient;
}

// Days from and to civil dates, by Howard Hinnant (http://howardhinnant.github.io/date_algorithms.html)
std::int64_t daysFromCivil(std::int64_t year, unsigned month, unsigned day) {
    year -= month <= 2 ? 1 : 0;
    const std::int64_t era = floorDivide(year, 400);
    const auto yearOfEra = static_cast<unsigned>(year - era * 400);
    const unsigned dayOfYear = (153 * (month > 2 ? month - 3 : month + 9) + 2) / 5 + day - 1;
    const unsigned dayOfEra = yearOfEra * 365 + yearOfEra / 4 - yearOfEra / 100 + dayOfYear;
    return era * 146097 + static_cast<std::int64_t>(dayOfEra) - 719468;
}

void civilFromDays(std::int64_t days, UtcDateTime& dateTime) {
    days += 719468;
    const std::int64_t era = floorDivide(days, 146097);
    const auto dayOfEra = static_cast<unsigned>(days - era * 146097);
    const unsigned yearOfEra = (dayOfEra - dayOfEra / 1460 + dayOfEra / 36524 - dayOfEra / 146096) / 365;
    const unsigned dayOfYear = dayOfEra - (365 * yearOfEra + yearOfEra / 4 - yearOfEra / 100);
    const unsigned monthPart = (5 * dayOfYear + 2) / 153;
    dateTime.day = dayOfYear - (153 * monthPart + 2) / 5 + 1;
    dateTime.month = monthPart < 10 ? monthPart + 3 : monthPart - 9;
    dateTime.year = static_cast<std::int64_t>(yearOfEra) + era * 400 + (dateTime.month <= 2 ? 1 : 0);
}

bool isLeapYear(std::int64_t year) {
    return (year % 4 == 0 && year % 100 != 0) || year % 400 == 0;
}

unsigned daysInMonth(std::int64_t year, unsigned month) {
    static constexpr std::array<unsigned, 12> DAYS{31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    return month == 2 && isLeapYear(year) ? 29 : DAYS[month - 1];
}

char lowercase(char character) {
    return character >= 'A' && character <= 'Z' ? static_cast<char>(character - 'A' + 'a') : character;
}

/// Reads a text from left to right, as a java.time formatter parses it.
class Scanner {
public:
    explicit Scanner(std::string_view text) : text_(text) {
    }

    bool atEnd() const {
        return position_ == text_.size();
    }

    bool literal(std::string_view expected) {
        if (text_.substr(position_, expected.size()).size() != expected.size()) {
            return false;
        }
        for (std::size_t index = 0; index < expected.size(); ++index) {
            if (lowercase(text_[position_ + index]) != lowercase(expected[index])) {
                return false;
            }
        }
        position_ += expected.size();
        return true;
    }

    /// A number of minimumDigits to maximumDigits decimal digits.
    std::optional<std::int64_t> number(std::size_t minimumDigits, std::size_t maximumDigits) {
        std::int64_t value = 0;
        std::size_t count = 0;
        while (count < maximumDigits && position_ < text_.size() && text_[position_] >= '0' && text_[position_] <= '9') {
            value = value * 10 + (text_[position_] - '0');
            ++position_;
            ++count;
        }
        if (count < minimumDigits) {
            return std::nullopt;
        }
        return value;
    }

    /// The index of the name the text continues with, compared in any case.
    template <std::size_t Size>
    std::optional<unsigned> name(const std::array<std::string_view, Size>& names) {
        for (unsigned index = 0; index < Size; ++index) {
            if (literal(names[index])) {
                return index;
            }
        }
        return std::nullopt;
    }

    char peek() const {
        return atEnd() ? '\0' : text_[position_];
    }

    void skip() {
        ++position_;
    }

private:
    std::string_view text_;
    std::size_t position_ = 0;
};

/// An offset written as +HHMM or +HH:MM, in seconds east of UTC.
std::optional<std::int64_t> offsetSeconds(Scanner& scanner, bool colon) {
    const char sign = scanner.peek();
    if (sign != '+' && sign != '-') {
        return std::nullopt;
    }
    scanner.skip();
    const auto hours = scanner.number(2, 2);
    if (!hours || (colon && !scanner.literal(":"))) {
        return std::nullopt;
    }
    const auto minutes = scanner.number(2, 2);
    if (!minutes || *hours > 18 || *minutes > 59) {
        return std::nullopt;
    }
    const std::int64_t seconds = *hours * 3600 + *minutes * 60;
    return sign == '-' ? -seconds : seconds;
}

}

Instant now() {
    return std::chrono::time_point_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now());
}

Clock systemClock() {
    return [] { return now(); };
}

Clock fixedClock(Instant instant) {
    return [instant] { return instant; };
}

UtcDateTime utcDateTime(Instant instant) {
    const std::int64_t nanos = instant.time_since_epoch().count();
    const std::int64_t seconds = floorDivide(nanos, NANOS_PER_SECOND);
    const std::int64_t days = floorDivide(seconds, SECONDS_PER_DAY);
    const std::int64_t secondOfDay = seconds - days * SECONDS_PER_DAY;
    UtcDateTime dateTime{};
    civilFromDays(days, dateTime);
    dateTime.hour = static_cast<unsigned>(secondOfDay / 3600);
    dateTime.minute = static_cast<unsigned>(secondOfDay % 3600 / 60);
    dateTime.second = static_cast<unsigned>(secondOfDay % 60);
    dateTime.nanosecond = nanos - seconds * NANOS_PER_SECOND;
    return dateTime;
}

std::optional<Instant> instantOf(const UtcDateTime& dateTime) {
    if (dateTime.year < FIRST_YEAR || dateTime.year > LAST_YEAR || dateTime.month < 1 || dateTime.month > 12
        || dateTime.day < 1 || dateTime.day > daysInMonth(dateTime.year, dateTime.month) || dateTime.hour > 23
        || dateTime.minute > 59 || dateTime.second > 59 || dateTime.nanosecond < 0
        || dateTime.nanosecond >= NANOS_PER_SECOND) {
        return std::nullopt;
    }
    const std::int64_t days = daysFromCivil(dateTime.year, dateTime.month, dateTime.day);
    const std::int64_t seconds = days * SECONDS_PER_DAY + dateTime.hour * 3600 + dateTime.minute * 60 + dateTime.second;
    return Instant(std::chrono::nanoseconds(seconds * NANOS_PER_SECOND + dateTime.nanosecond));
}

std::string utcDay(Instant instant) {
    const UtcDateTime dateTime = utcDateTime(instant);
    char text[16];
    std::snprintf(text, sizeof text, "%04lld%02u%02u", static_cast<long long>(dateTime.year), dateTime.month,
                  dateTime.day);
    return text;
}

std::string utcHour(Instant instant) {
    char text[4];
    std::snprintf(text, sizeof text, "%02u", utcDateTime(instant).hour);
    return text;
}

std::optional<Instant> parseInstant(std::string_view text) {
    Scanner scanner(text);
    UtcDateTime dateTime{};
    const auto year = scanner.number(4, 4);
    if (!year || !scanner.literal("-")) {
        return std::nullopt;
    }
    const auto month = scanner.number(2, 2);
    if (!month || !scanner.literal("-")) {
        return std::nullopt;
    }
    const auto day = scanner.number(2, 2);
    if (!day || !scanner.literal("T")) {
        return std::nullopt;
    }
    const auto hour = scanner.number(2, 2);
    if (!hour || !scanner.literal(":")) {
        return std::nullopt;
    }
    const auto minute = scanner.number(2, 2);
    if (!minute || !scanner.literal(":")) {
        return std::nullopt;
    }
    const auto second = scanner.number(2, 2);
    if (!second) {
        return std::nullopt;
    }
    std::int64_t nanosecond = 0;
    if (scanner.literal(".")) {
        std::int64_t scale = NANOS_PER_SECOND;
        std::size_t digits = 0;
        while (scanner.peek() >= '0' && scanner.peek() <= '9' && digits < 9) {
            scale /= 10;
            nanosecond += (scanner.peek() - '0') * scale;
            scanner.skip();
            ++digits;
        }
        if (digits == 0) {
            return std::nullopt;
        }
    }
    std::int64_t offset = 0;
    if (!scanner.literal("Z")) {
        const auto parsed = offsetSeconds(scanner, true);
        if (!parsed) {
            return std::nullopt;
        }
        offset = *parsed;
    }
    if (!scanner.atEnd()) {
        return std::nullopt;
    }
    dateTime = {*year, static_cast<unsigned>(*month), static_cast<unsigned>(*day), static_cast<unsigned>(*hour),
                static_cast<unsigned>(*minute), static_cast<unsigned>(*second), nanosecond};
    const auto local = instantOf(dateTime);
    if (!local) {
        return std::nullopt;
    }
    return *local - std::chrono::seconds(offset);
}

std::string formatInstant(Instant instant) {
    const UtcDateTime dateTime = utcDateTime(instant);
    char text[48];
    std::snprintf(text, sizeof text, "%04lld-%02u-%02uT%02u:%02u:%02u", static_cast<long long>(dateTime.year),
                  dateTime.month, dateTime.day, dateTime.hour, dateTime.minute, dateTime.second);
    std::string result = text;
    if (dateTime.nanosecond != 0) {
        char fraction[32];
        if (dateTime.nanosecond % 1'000'000 == 0) {
            std::snprintf(fraction, sizeof fraction, ".%03lld", static_cast<long long>(dateTime.nanosecond / 1'000'000));
        } else if (dateTime.nanosecond % 1000 == 0) {
            std::snprintf(fraction, sizeof fraction, ".%06lld", static_cast<long long>(dateTime.nanosecond / 1000));
        } else {
            std::snprintf(fraction, sizeof fraction, ".%09lld", static_cast<long long>(dateTime.nanosecond));
        }
        result += fraction;
    }
    return result + "Z";
}

std::optional<Instant> parseHttpDate(std::string_view text) {
    Scanner scanner(text);
    std::optional<unsigned> weekday;
    if (std::isalpha(static_cast<unsigned char>(scanner.peek()))) {
        weekday = scanner.name(WEEKDAYS);
        if (!weekday || !scanner.literal(", ")) {
            return std::nullopt;
        }
    }
    const auto day = scanner.number(1, 2);
    if (!day || !scanner.literal(" ")) {
        return std::nullopt;
    }
    const auto month = scanner.name(MONTHS);
    if (!month || !scanner.literal(" ")) {
        return std::nullopt;
    }
    const auto year = scanner.number(4, 4);
    if (!year || !scanner.literal(" ")) {
        return std::nullopt;
    }
    const auto hour = scanner.number(2, 2);
    if (!hour || !scanner.literal(":")) {
        return std::nullopt;
    }
    const auto minute = scanner.number(2, 2);
    if (!minute) {
        return std::nullopt;
    }
    std::int64_t second = 0;
    if (scanner.literal(":")) {
        const auto seconds = scanner.number(2, 2);
        if (!seconds) {
            return std::nullopt;
        }
        second = *seconds;
    }
    if (!scanner.literal(" ")) {
        return std::nullopt;
    }
    std::int64_t offset = 0;
    if (!scanner.literal("GMT")) {
        const auto parsed = offsetSeconds(scanner, false);
        if (!parsed) {
            return std::nullopt;
        }
        offset = *parsed;
    }
    if (!scanner.atEnd()) {
        return std::nullopt;
    }
    const UtcDateTime dateTime{*year, *month + 1, static_cast<unsigned>(*day), static_cast<unsigned>(*hour),
                               static_cast<unsigned>(*minute), static_cast<unsigned>(second), 0};
    const auto local = instantOf(dateTime);
    if (!local) {
        return std::nullopt;
    }
    if (weekday) {
        const std::int64_t days = daysFromCivil(dateTime.year, dateTime.month, dateTime.day);
        // 1 January 1970 was a Thursday
        if (static_cast<unsigned>(((days + 4) % 7 + 7) % 7) != *weekday) {
            return std::nullopt;
        }
    }
    return *local - std::chrono::seconds(offset);
}

std::string formatHttpDate(Instant instant) {
    const UtcDateTime dateTime = utcDateTime(instant);
    const std::int64_t days = daysFromCivil(dateTime.year, dateTime.month, dateTime.day);
    const auto weekday = static_cast<std::size_t>(((days + 4) % 7 + 7) % 7);
    char text[48];
    std::snprintf(text, sizeof text, "%s, %02u %s %04lld %02u:%02u:%02u GMT", WEEKDAYS[weekday].data(), dateTime.day,
                  MONTHS[dateTime.month - 1].data(), static_cast<long long>(dateTime.year), dateTime.hour,
                  dateTime.minute, dateTime.second);
    return text;
}

}
