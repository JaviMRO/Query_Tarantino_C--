#include "tarantino/platform/Utf8.hpp"

#include <gtest/gtest.h>

namespace tarantino::platform::utf8 {
namespace {

TEST(Utf8Test, acceptsWellFormedText) {
    EXPECT_TRUE(isValid(""));
    EXPECT_TRUE(isValid("plain ASCII text that is longer than eight bytes"));
    EXPECT_TRUE(isValid("café 漢字 \U0001D49C"));
}

TEST(Utf8Test, rejectsMalformedText) {
    EXPECT_FALSE(isValid("\xC0\x80"));          // overlong
    EXPECT_FALSE(isValid("\xED\xA0\x80"));      // surrogate
    EXPECT_FALSE(isValid("\xF4\x90\x80\x80"));  // above U+10FFFF
    EXPECT_FALSE(isValid("abc\xE2\x82"));       // truncated
    EXPECT_FALSE(isValid("\x80"));              // lone continuation byte
}

TEST(Utf8Test, replacesEachMaximalSubpartOfMalformedTextWithOneReplacementCharacter) {
    EXPECT_EQ("a�(b", sanitized("a\xC3(b"));
    EXPECT_EQ("�A", sanitized("\xE1\x80" "A"));
    EXPECT_EQ("�", sanitized("\xF0\x9F\x98"));
    EXPECT_EQ("���", sanitized("\xED\xA0\x80"));
    EXPECT_EQ("café", sanitized("café"));
}

TEST(Utf8Test, decodesCodePointsForwardAndBackward) {
    const std::string_view text = "aé漢\U0001D49C";
    std::size_t index = 0;
    EXPECT_EQ(U'a', next(text, index));
    EXPECT_EQ(U'é', next(text, index));
    EXPECT_EQ(U'漢', next(text, index));
    EXPECT_EQ(U'\U0001D49C', next(text, index));
    EXPECT_EQ(text.size(), index);
    EXPECT_EQ(U'\U0001D49C', previous(text, index));
    EXPECT_EQ(U'漢', previous(text, index));
    EXPECT_EQ(U'é', previous(text, index));
    EXPECT_EQ(U'a', previous(text, index));
    EXPECT_EQ(0u, index);
}

TEST(Utf8Test, encodesAndCountsCodePoints) {
    std::string text;
    append(text, U'a');
    append(text, U'é');
    append(text, U'漢');
    append(text, U'\U0001D49C');
    EXPECT_EQ("aé漢\U0001D49C", text);
    EXPECT_EQ(4u, codePointCount(text));
    EXPECT_EQ(4u, sequenceLength(static_cast<unsigned char>(text[6])));
}

}
}
