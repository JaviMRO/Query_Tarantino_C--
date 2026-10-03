#include "tarantino/indexer/model/terms/Tokenizer.hpp"

#include <gtest/gtest.h>

namespace tarantino::indexer::model {
namespace {

using Frequencies = platform::StringMap<int>;

class TokenizerTest : public ::testing::Test {
protected:
    Tokenizer tokenizer{platform::StringSet{"the", "of"}};
};

TEST_F(TokenizerTest, countsLowercasedTermsWithoutStopwordsOrShortTokens) {
    const TermOccurrences occurrences = tokenizer.occurrences(5, "The Island of the island, a SHIPWRECK!");

    EXPECT_EQ(5, occurrences.bookId);
    EXPECT_EQ((Frequencies{{"island", 2}, {"shipwreck", 1}}), occurrences.frequencies);
}

TEST_F(TokenizerTest, keepsAccentedLettersAndSplitsOnDigitsAndApostrophes) {
    const TermOccurrences occurrences = tokenizer.occurrences(5, "Café don't 1984year");

    EXPECT_EQ((Frequencies{{"café", 1}, {"don", 1}, {"year", 1}}), occurrences.frequencies);
}

TEST_F(TokenizerTest, measuresTermLengthInCodePoints) {
    const TermOccurrences occurrences = tokenizer.occurrences(5, "\U0001D49C \U0001D49C\U0001D49C");

    EXPECT_EQ((Frequencies{{"\U0001D49C\U0001D49C", 1}}), occurrences.frequencies);
}

TEST_F(TokenizerTest, lowercasesTheWholeTextAtOnce) {
    // A final capital sigma becomes ς, and İ becomes i and a combining dot above, which is not a letter
    const TermOccurrences occurrences = tokenizer.occurrences(5, "ΟΔΟΣ İstanbul");

    EXPECT_EQ((Frequencies{{"οδος", 1}, {"stanbul", 1}}), occurrences.frequencies);
}

TEST_F(TokenizerTest, findsNoTermInAnEmptyBody) {
    EXPECT_TRUE(tokenizer.occurrences(5, "").frequencies.empty());
    EXPECT_TRUE(tokenizer.occurrences(5, "1 - 2 ... a").frequencies.empty());
}

}
}
