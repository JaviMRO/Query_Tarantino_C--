#include "tarantino/platform/Unicode.hpp"

#include <gtest/gtest.h>

namespace tarantino::platform::unicode {
namespace {

TEST(UnicodeTest, lowercasesWithTheRootLocale) {
    EXPECT_EQ("the island", toLowerCase("The ISLAND"));
    EXPECT_EQ("écume façade cæsar", toLowerCase("ÉCUME FAÇADE CÆSAR"));
    // Root, not Turkish: I becomes i
    EXPECT_EQ("istanbul", toLowerCase("ISTANBUL"));
}

TEST(UnicodeTest, lowercasesAFinalSigmaByItsContext) {
    // ΟΔΟΣ becomes οδος with a final ς, and ΣΑΣ becomes σας
    EXPECT_EQ("οδος", toLowerCase("ΟΔΟΣ"));
    EXPECT_EQ("σας", toLowerCase("ΣΑΣ"));
}

TEST(UnicodeTest, lowercasesIntoLongerText) {
    // İ becomes i and a combining dot above, one byte longer in UTF-8
    EXPECT_EQ("i̇stanbul", toLowerCase("İstanbul"));
    EXPECT_EQ("i̇i̇i̇", toLowerCase("İİİ"));
}

TEST(UnicodeTest, findsTheLettersOfCategoryL) {
    for (const char32_t letter : {U'a', U'Z', U'é', U'漢', U'\U0001D49C', U'ǅ', U'ʰ', U'ſ'}) {
        EXPECT_TRUE(isLetter(letter)) << static_cast<std::uint32_t>(letter);
    }
    for (const char32_t other : {U'1', U'-', U'\'', U' ', U'_', U'¹', U'́', U'—', U' '}) {
        EXPECT_FALSE(isLetter(other)) << static_cast<std::uint32_t>(other);
    }
}

TEST(UnicodeTest, tellsItsVersions) {
    EXPECT_FALSE(unicodeVersion().empty());
    EXPECT_FALSE(icuVersion().empty());
}

}
}
