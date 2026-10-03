#pragma once

#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

/// What the Java services take from java.lang.String and java.nio.file.Files: the text rules of SPEC §1.
namespace tarantino::platform {

/// The hash of std::string that also takes a std::string_view, so that hash maps and sets of strings can be looked up
/// with a piece of a text without copying it.
struct StringHash {
    using is_transparent = void;

    std::size_t operator()(std::string_view text) const noexcept {
        return std::hash<std::string_view>{}(text);
    }
};

/// A hash set of strings (Java's HashSet<String>) that a std::string_view can look up.
using StringSet = std::unordered_set<std::string, StringHash, std::equal_to<>>;

/// A hash map from strings (Java's HashMap<String, V>) that a std::string_view can look up.
template <typename Value>
using StringMap = std::unordered_map<std::string, Value, StringHash, std::equal_to<>>;

/// Whether Java's Character.isWhitespace holds for the code point, which is what String.strip removes (SPEC §1):
/// U+0009 to U+000D, U+001C to U+001F, U+0020, U+1680, U+2000 to U+2006, U+2008 to U+200A, U+2028, U+2029, U+205F
/// and U+3000, but not U+0085, U+00A0, U+2007 or U+202F.
bool isJavaWhitespace(char32_t codePoint);

/// The text without the leading and trailing whitespace that Java's String.strip removes (SPEC §1).
std::string_view strip(std::string_view text);

/// The lines of a text. Only \n ends a line (SPEC §1), and a final \n does not start another, empty, line.
std::vector<std::string_view> lines(std::string_view text);

/// A whole number written only with decimal digits, such as a book id in a control file; empty if the text has any
/// other character or the number does not fit an int.
std::optional<int> parseDigits(std::string_view text);

/// The text with every occurrence of a non-empty string replaced by another, as Java's String.replace does.
std::string replaceAll(std::string_view text, std::string_view target, std::string_view replacement);

}
