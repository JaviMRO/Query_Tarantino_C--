#include "tarantino/platform/Utf8.hpp"

#include <cstdint>
#include <cstring>

namespace tarantino::platform::utf8 {

namespace {

constexpr std::uint64_t NON_ASCII_BYTES = 0x8080808080808080ULL;

/// The length of the well-formed sequence that starts at index, or 0 if it is malformed, in which case subpart is set
/// to the length of its maximal subpart: the longest start of a well-formed sequence, at least one byte.
std::size_t wellFormedLength(std::string_view bytes, std::size_t index, std::size_t& subpart) {
    const std::size_t available = bytes.size() - index;
    const auto byte = [&](std::size_t offset) { return static_cast<unsigned char>(bytes[index + offset]); };
    const unsigned char lead = byte(0);
    if (lead < 0x80) {
        return 1;
    }
    std::size_t length = 0;
    unsigned char low = 0x80;
    unsigned char high = 0xBF;
    if (lead >= 0xC2 && lead <= 0xDF) {
        length = 2;
    } else if (lead == 0xE0) {
        length = 3;
        low = 0xA0;
    } else if ((lead >= 0xE1 && lead <= 0xEC) || lead == 0xEE || lead == 0xEF) {
        length = 3;
    } else if (lead == 0xED) {
        length = 3;
        high = 0x9F;
    } else if (lead == 0xF0) {
        length = 4;
        low = 0x90;
    } else if (lead >= 0xF1 && lead <= 0xF3) {
        length = 4;
    } else if (lead == 0xF4) {
        length = 4;
        high = 0x8F;
    } else {
        subpart = 1;
        return 0;
    }
    if (available < 2 || byte(1) < low || byte(1) > high) {
        subpart = 1;
        return 0;
    }
    for (std::size_t offset = 2; offset < length; ++offset) {
        if (available <= offset || byte(offset) < 0x80 || byte(offset) > 0xBF) {
            subpart = offset;
            return 0;
        }
    }
    return length;
}

char32_t decoded(std::string_view bytes, std::size_t index, std::size_t length) {
    const auto byte = [&](std::size_t offset) { return static_cast<char32_t>(static_cast<unsigned char>(bytes[index + offset])); };
    switch (length) {
        case 1:
            return byte(0);
        case 2:
            return ((byte(0) & 0x1F) << 6) | (byte(1) & 0x3F);
        case 3:
            return ((byte(0) & 0x0F) << 12) | ((byte(1) & 0x3F) << 6) | (byte(2) & 0x3F);
        default:
            return ((byte(0) & 0x07) << 18) | ((byte(1) & 0x3F) << 12) | ((byte(2) & 0x3F) << 6) | (byte(3) & 0x3F);
    }
}

/// Skips the ASCII bytes from index, eight at a time while it can.
std::size_t afterAscii(std::string_view bytes, std::size_t index) {
    while (index + 8 <= bytes.size()) {
        std::uint64_t word;
        std::memcpy(&word, bytes.data() + index, sizeof word);
        if ((word & NON_ASCII_BYTES) != 0) {
            break;
        }
        index += 8;
    }
    while (index < bytes.size() && static_cast<unsigned char>(bytes[index]) < 0x80) {
        ++index;
    }
    return index;
}

}

bool isValid(std::string_view bytes) {
    std::size_t index = afterAscii(bytes, 0);
    while (index < bytes.size()) {
        std::size_t subpart = 0;
        const std::size_t length = wellFormedLength(bytes, index, subpart);
        if (length == 0) {
            return false;
        }
        index = afterAscii(bytes, index + length);
    }
    return true;
}

std::string sanitized(std::string bytes) {
    if (isValid(bytes)) {
        return bytes;
    }
    std::string text;
    text.reserve(bytes.size() + 16);
    std::size_t index = 0;
    while (index < bytes.size()) {
        std::size_t subpart = 0;
        const std::size_t length = wellFormedLength(bytes, index, subpart);
        if (length == 0) {
            append(text, REPLACEMENT_CHARACTER);
            index += subpart;
        } else {
            text.append(bytes, index, length);
            index += length;
        }
    }
    return text;
}

char32_t next(std::string_view text, std::size_t& index) {
    std::size_t subpart = 0;
    const std::size_t length = wellFormedLength(text, index, subpart);
    if (length == 0) {
        index += subpart;
        return REPLACEMENT_CHARACTER;
    }
    const char32_t codePoint = decoded(text, index, length);
    index += length;
    return codePoint;
}

char32_t previous(std::string_view text, std::size_t& index) {
    std::size_t start = index - 1;
    while (start > 0 && index - start < 4 && (static_cast<unsigned char>(text[start]) & 0xC0) == 0x80) {
        --start;
    }
    std::size_t subpart = 0;
    const std::size_t length = wellFormedLength(text, start, subpart);
    if (length == 0 || start + length != index) {
        --index;
        return REPLACEMENT_CHARACTER;
    }
    index = start;
    return decoded(text, start, length);
}

void append(std::string& text, char32_t codePoint) {
    if (codePoint < 0x80) {
        text.push_back(static_cast<char>(codePoint));
    } else if (codePoint < 0x800) {
        text.push_back(static_cast<char>(0xC0 | (codePoint >> 6)));
        text.push_back(static_cast<char>(0x80 | (codePoint & 0x3F)));
    } else if (codePoint < 0x10000) {
        text.push_back(static_cast<char>(0xE0 | (codePoint >> 12)));
        text.push_back(static_cast<char>(0x80 | ((codePoint >> 6) & 0x3F)));
        text.push_back(static_cast<char>(0x80 | (codePoint & 0x3F)));
    } else {
        text.push_back(static_cast<char>(0xF0 | (codePoint >> 18)));
        text.push_back(static_cast<char>(0x80 | ((codePoint >> 12) & 0x3F)));
        text.push_back(static_cast<char>(0x80 | ((codePoint >> 6) & 0x3F)));
        text.push_back(static_cast<char>(0x80 | (codePoint & 0x3F)));
    }
}

std::size_t codePointCount(std::string_view text) {
    std::size_t count = 0;
    for (const char byte : text) {
        if ((static_cast<unsigned char>(byte) & 0xC0) != 0x80) {
            ++count;
        }
    }
    return count;
}

std::size_t sequenceLength(unsigned char firstByte) {
    if (firstByte >= 0xC2 && firstByte <= 0xDF) {
        return 2;
    }
    if (firstByte >= 0xE0 && firstByte <= 0xEF) {
        return 3;
    }
    if (firstByte >= 0xF0 && firstByte <= 0xF4) {
        return 4;
    }
    return 1;
}

}
