#include "tarantino/platform/Sha256.hpp"

#include <gtest/gtest.h>

namespace tarantino::platform {
namespace {

std::string sha256Hex(std::string_view text) {
    const auto digest = sha256(text);
    return hex(digest.data(), digest.size());
}

TEST(Sha256Test, digestsTheTestVectorsOfFips180) {
    EXPECT_EQ("e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855", sha256Hex(""));
    EXPECT_EQ("ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad", sha256Hex("abc"));
    // 56 bytes: the padding takes a second block
    EXPECT_EQ("248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1",
              sha256Hex("abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq"));
    EXPECT_EQ("cdc76e5c9914fb9281a1c7e284d73e67f1809a48a497200e046d39ccc7112cd0", sha256Hex(std::string(1'000'000, 'a')));
}

TEST(Sha256Test, digestsTheUtf8BytesOfATerm) {
    std::string term;
    for (int character = 0; character < 100; ++character) {
        term += "漢";
    }
    EXPECT_EQ("1ba4c06e4c0345f9bc48eed7d09f0c877798ff0dc8d3d0a0fad31cfcba7ca564", sha256Hex(term));
}

TEST(Sha256Test, writesHexadecimalInEitherCase) {
    const std::uint8_t bytes[] = {0x00, 0xC3, 0xA9, 0xFF};
    EXPECT_EQ("00c3a9ff", hex(bytes, 4));
    EXPECT_EQ("00C3A9FF", hex(bytes, 4, true));
}

}
}
