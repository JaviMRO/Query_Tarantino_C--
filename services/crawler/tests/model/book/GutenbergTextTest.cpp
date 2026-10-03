#include "tarantino/crawler/model/book/GutenbergText.hpp"
#include "tarantino/crawler/model/failure/DownloadException.hpp"

#include <gtest/gtest.h>

namespace tarantino::crawler::model {
namespace {

constexpr const char* RAW = "Title: Pride and Prejudice\r\n"
                            "Author: Jane Austen\r\n"
                            "\r\n"
                            "*** START OF THE PROJECT GUTENBERG EBOOK PRIDE AND PREJUDICE ***\r\n"
                            "\r\n"
                            "It is a truth universally acknowledged.\r\n"
                            "\r\n"
                            "*** END OF THE PROJECT GUTENBERG EBOOK PRIDE AND PREJUDICE ***\r\n"
                            "License text.\r\n";

TEST(GutenbergTextTest, splitsHeaderAndBodyDiscardingMarkersAndFooter) {
    const BookText book = GutenbergText::bookText(1342, RAW);

    EXPECT_EQ(1342, book.bookId);
    EXPECT_EQ("Title: Pride and Prejudice\nAuthor: Jane Austen", book.header);
    EXPECT_EQ("It is a truth universally acknowledged.", book.body);
}

TEST(GutenbergTextTest, acceptsLegacyThisMarkers) {
    const char* raw = "Header\n*** START OF THIS PROJECT GUTENBERG EBOOK X ***\nBody\n*** END OF THIS PROJECT GUTENBERG EBOOK X ***";

    EXPECT_EQ("Body", GutenbergText::bookText(1, raw).body);
}

TEST(GutenbergTextTest, failsWhenMarkersAreMissing) {
    try {
        GutenbergText::bookText(1, "no markers");
        FAIL() << "a text without markers was split";
    } catch (const DownloadException& error) {
        EXPECT_EQ(FailureReason::MissingMarkers, error.reason());
    }
}

TEST(GutenbergTextTest, takesTheEarliestOfTheLiteralFormsOfEachMarker) {
    const char* raw = "H\n****START OF THE PROJECT GUTENBERG EBOOK A\n"
                      "B *** END OF THIS PROJECT GUTENBERG EBOOK X *** END OF THE PROJECT GUTENBERG EBOOK";

    const BookText book = GutenbergText::bookText(1, raw);

    EXPECT_EQ("H\n*", book.header);
    EXPECT_EQ("B", book.body);
}

TEST(GutenbergTextTest, needsALineEndAfterTheStartMarker) {
    EXPECT_THROW(GutenbergText::bookText(1, "H\n*** START OF THE PROJECT GUTENBERG EBOOK X *** END OF THE PROJECT GUTENBERG EBOOK"),
                 DownloadException);
}

}
}
