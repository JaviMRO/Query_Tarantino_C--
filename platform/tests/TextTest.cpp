#include "tarantino/platform/Text.hpp"

#include <gtest/gtest.h>

namespace tarantino::platform {
namespace {

TEST(TextTest, stripsTheWhitespaceOfJavasStringStrip) {
    // U+2003 EM SPACE and U+3000 IDEOGRAPHIC SPACE are whitespace for Character.isWhitespace
    EXPECT_EQ("Title: F", strip("\t  Title: F　\n\r"));
    EXPECT_EQ("", strip(" \t\n"));
    EXPECT_EQ("", strip(""));
}

TEST(TextTest, keepsTheSpacesThatJavaDoesNotStrip) {
    // U+00A0, U+2007, U+202F and U+0085 are not whitespace for Character.isWhitespace (SPEC §1)
    EXPECT_EQ(" Title  ", strip(" Title   "));
    EXPECT_EQ("\u0085Body", strip("\u0085Body"));
}

TEST(TextTest, splitsLinesOnlyAtLineFeeds) {
    EXPECT_EQ((std::vector<std::string_view>{"a\rb", "", "c"}), lines("a\rb\n\nc\n"));
    EXPECT_EQ((std::vector<std::string_view>{"a b"}), lines("a b"));
    EXPECT_TRUE(lines("").empty());
}

TEST(TextTest, readsOnlyNumbersWrittenWithDigits) {
    EXPECT_EQ(1342, parseDigits("1342"));
    EXPECT_EQ(0, parseDigits("0"));
    EXPECT_EQ(std::nullopt, parseDigits("+5"));
    EXPECT_EQ(std::nullopt, parseDigits("13x"));
    EXPECT_EQ(std::nullopt, parseDigits(""));
    EXPECT_EQ(std::nullopt, parseDigits("99999999999"));
}

TEST(TextTest, replacesEveryOccurrence) {
    EXPECT_EQ("a\nb\nc", replaceAll("a\r\nb\r\nc", "\r\n", "\n"));
    EXPECT_EQ("1342.body.txt", replaceAll("1342.header.txt", "header.txt", "body.txt"));
    EXPECT_EQ("unchanged", replaceAll("unchanged", "", "x"));
}

}
}
