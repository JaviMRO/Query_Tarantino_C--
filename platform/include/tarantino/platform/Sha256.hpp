#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <string_view>

namespace tarantino::platform {

/// The SHA-256 digest of the bytes (FIPS 180-4), as Java's MessageDigest.getInstance("SHA-256") computes it.
std::array<std::uint8_t, 32> sha256(std::string_view bytes);

/// The bytes in hexadecimal, two digits each, in lowercase as HexFormat.of() or in uppercase as withUpperCase().
std::string hex(const std::uint8_t* bytes, std::size_t size, bool uppercase = false);

}
