#include "tarantino/platform/Sha256.hpp"

namespace tarantino::platform {

namespace {

constexpr std::array<std::uint32_t, 64> ROUND_CONSTANTS{
        0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
        0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
        0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
        0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
        0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
        0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
        0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
        0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2};

constexpr std::uint32_t rotateRight(std::uint32_t value, int bits) {
    return (value >> bits) | (value << (32 - bits));
}

void compress(std::array<std::uint32_t, 8>& state, const std::uint8_t* block) {
    std::array<std::uint32_t, 64> schedule{};
    for (int word = 0; word < 16; ++word) {
        schedule[word] = static_cast<std::uint32_t>(block[4 * word]) << 24 | static_cast<std::uint32_t>(block[4 * word + 1]) << 16
                         | static_cast<std::uint32_t>(block[4 * word + 2]) << 8 | static_cast<std::uint32_t>(block[4 * word + 3]);
    }
    for (int word = 16; word < 64; ++word) {
        const std::uint32_t s0 = rotateRight(schedule[word - 15], 7) ^ rotateRight(schedule[word - 15], 18) ^ (schedule[word - 15] >> 3);
        const std::uint32_t s1 = rotateRight(schedule[word - 2], 17) ^ rotateRight(schedule[word - 2], 19) ^ (schedule[word - 2] >> 10);
        schedule[word] = schedule[word - 16] + s0 + schedule[word - 7] + s1;
    }
    std::uint32_t a = state[0], b = state[1], c = state[2], d = state[3];
    std::uint32_t e = state[4], f = state[5], g = state[6], h = state[7];
    for (int round = 0; round < 64; ++round) {
        const std::uint32_t s1 = rotateRight(e, 6) ^ rotateRight(e, 11) ^ rotateRight(e, 25);
        const std::uint32_t choice = (e & f) ^ (~e & g);
        const std::uint32_t first = h + s1 + choice + ROUND_CONSTANTS[round] + schedule[round];
        const std::uint32_t s0 = rotateRight(a, 2) ^ rotateRight(a, 13) ^ rotateRight(a, 22);
        const std::uint32_t majority = (a & b) ^ (a & c) ^ (b & c);
        const std::uint32_t second = s0 + majority;
        h = g;
        g = f;
        f = e;
        e = d + first;
        d = c;
        c = b;
        b = a;
        a = first + second;
    }
    state[0] += a;
    state[1] += b;
    state[2] += c;
    state[3] += d;
    state[4] += e;
    state[5] += f;
    state[6] += g;
    state[7] += h;
}

}

std::array<std::uint8_t, 32> sha256(std::string_view bytes) {
    std::array<std::uint32_t, 8> state{0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
                                       0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19};
    const auto* data = reinterpret_cast<const std::uint8_t*>(bytes.data());
    const std::size_t size = bytes.size();
    std::size_t offset = 0;
    for (; offset + 64 <= size; offset += 64) {
        compress(state, data + offset);
    }
    // The last bytes, the 1 bit, zeros and the length in bits, in one or two final blocks
    std::array<std::uint8_t, 128> tail{};
    const std::size_t remaining = size - offset;
    for (std::size_t index = 0; index < remaining; ++index) {
        tail[index] = data[offset + index];
    }
    tail[remaining] = 0x80;
    const std::size_t tailSize = remaining + 1 + 8 <= 64 ? 64 : 128;
    const std::uint64_t bits = static_cast<std::uint64_t>(size) * 8;
    for (int byte = 0; byte < 8; ++byte) {
        tail[tailSize - 1 - byte] = static_cast<std::uint8_t>(bits >> (8 * byte));
    }
    compress(state, tail.data());
    if (tailSize == 128) {
        compress(state, tail.data() + 64);
    }
    std::array<std::uint8_t, 32> digest{};
    for (int word = 0; word < 8; ++word) {
        digest[4 * word] = static_cast<std::uint8_t>(state[word] >> 24);
        digest[4 * word + 1] = static_cast<std::uint8_t>(state[word] >> 16);
        digest[4 * word + 2] = static_cast<std::uint8_t>(state[word] >> 8);
        digest[4 * word + 3] = static_cast<std::uint8_t>(state[word]);
    }
    return digest;
}

std::string hex(const std::uint8_t* bytes, std::size_t size, bool uppercase) {
    const char* digits = uppercase ? "0123456789ABCDEF" : "0123456789abcdef";
    std::string text;
    text.reserve(size * 2);
    for (std::size_t index = 0; index < size; ++index) {
        text.push_back(digits[bytes[index] >> 4]);
        text.push_back(digits[bytes[index] & 0x0F]);
    }
    return text;
}

}
