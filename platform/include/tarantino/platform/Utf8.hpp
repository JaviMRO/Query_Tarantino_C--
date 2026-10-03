#pragma once

#include <cstddef>
#include <string>
#include <string_view>

/// UTF-8, the encoding of every std::string of the project (SPEC §1, §14).
namespace tarantino::platform::utf8 {

/// The code point Java's decoder writes for each malformed sequence.
inline constexpr char32_t REPLACEMENT_CHARACTER = U'�';

/// Whether the bytes are well-formed UTF-8 (Unicode table 3-7: no overlong forms, surrogates or values above U+10FFFF).
bool isValid(std::string_view bytes);

/// The bytes with each malformed sequence replaced by U+FFFD, one per maximal subpart, as Java decodes a response body
/// (SPEC §4); well-formed text comes back unchanged.
std::string sanitized(std::string bytes);

/// Decodes the code point that starts at index and moves index past it. The text should be well-formed: a malformed
/// sequence is returned as U+FFFD and skipped as a whole maximal subpart.
char32_t next(std::string_view text, std::size_t& index);

/// Decodes the code point that ends right before index and moves index to its first byte. The text should be
/// well-formed: a malformed byte is returned as U+FFFD and skipped alone.
char32_t previous(std::string_view text, std::size_t& index);

/// Appends the UTF-8 bytes of a code point.
void append(std::string& text, char32_t codePoint);

/// The number of code points of well-formed text.
std::size_t codePointCount(std::string_view text);

/// The number of bytes of the code point whose first byte is given (1 for a byte that cannot start one).
std::size_t sequenceLength(unsigned char firstByte);

}
