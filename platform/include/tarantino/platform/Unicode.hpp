#pragma once

#include <string>
#include <string_view>

/// The Unicode rules of SPEC §7, through ICU (SPEC §14) and its C API, the one stable on every platform, including the
/// ICU that Windows ships.
namespace tarantino::platform::unicode {

/// The text lowercased at once with ICU's root locale and full mapping, which is context-sensitive (a final capital
/// sigma becomes ς) and may lengthen it (İ becomes i and U+0307): the mapping of
/// icu::UnicodeString::toLower(icu::Locale::getRoot()) and of Java's toLowerCase(Locale.ROOT), applied to UTF-8.
std::string toLowerCase(std::string_view text);

bool isLetterBeyondAscii(char32_t codePoint);

/// Whether the code point is a letter, of general category L: ICU's u_isalpha, as Java's Character.isLetter.
inline bool isLetter(char32_t codePoint) {
    if (codePoint < 0x80) {
        return (codePoint >= 'a' && codePoint <= 'z') || (codePoint >= 'A' && codePoint <= 'Z');
    }
    return isLetterBeyondAscii(codePoint);
}

/// The Unicode version of the ICU in use, such as "16.0", which SPEC §1 asks every implementation to record.
std::string unicodeVersion();

/// The version of the ICU in use, such as "78.2".
std::string icuVersion();

}
