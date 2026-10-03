#include "tarantino/platform/Text.hpp"

#include "tarantino/platform/Utf8.hpp"

#include <charconv>

namespace tarantino::platform {

bool isJavaWhitespace(char32_t codePoint) {
    return (codePoint >= 0x09 && codePoint <= 0x0D) || (codePoint >= 0x1C && codePoint <= 0x20) || codePoint == 0x1680
           || (codePoint >= 0x2000 && codePoint <= 0x2006) || (codePoint >= 0x2008 && codePoint <= 0x200A)
           || codePoint == 0x2028 || codePoint == 0x2029 || codePoint == 0x205F || codePoint == 0x3000;
}

std::string_view strip(std::string_view text) {
    std::size_t start = 0;
    while (start < text.size()) {
        std::size_t next = start;
        if (!isJavaWhitespace(utf8::next(text, next))) {
            break;
        }
        start = next;
    }
    std::size_t end = text.size();
    while (end > start) {
        std::size_t previous = end;
        if (!isJavaWhitespace(utf8::previous(text, previous))) {
            break;
        }
        end = previous;
    }
    return text.substr(start, end - start);
}

std::vector<std::string_view> lines(std::string_view text) {
    std::vector<std::string_view> result;
    std::size_t start = 0;
    while (start < text.size()) {
        const std::size_t end = text.find('\n', start);
        if (end == std::string_view::npos) {
            result.push_back(text.substr(start));
            break;
        }
        result.push_back(text.substr(start, end - start));
        start = end + 1;
    }
    return result;
}

std::optional<int> parseDigits(std::string_view text) {
    if (text.empty()) {
        return std::nullopt;
    }
    for (const char character : text) {
        if (character < '0' || character > '9') {
            return std::nullopt;
        }
    }
    int number = 0;
    const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), number);
    if (error != std::errc{} || end != text.data() + text.size()) {
        return std::nullopt;
    }
    return number;
}

std::string replaceAll(std::string_view text, std::string_view target, std::string_view replacement) {
    if (target.empty()) {
        return std::string(text);
    }
    std::string result;
    result.reserve(text.size());
    std::size_t start = 0;
    while (true) {
        const std::size_t found = text.find(target, start);
        if (found == std::string_view::npos) {
            result.append(text.substr(start));
            return result;
        }
        result.append(text.substr(start, found - start)).append(replacement);
        start = found + target.size();
    }
}

}
