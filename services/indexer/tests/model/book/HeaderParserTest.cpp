#include "tarantino/indexer/model/book/HeaderParser.hpp"

#include <gtest/gtest.h>

namespace tarantino::indexer::model {
namespace {

const std::filesystem::path BODY = std::filesystem::path("datalake") / "20250925" / "14" / "5.body.txt";

TEST(HeaderParserTest, extractsTitleAuthorAndLanguage) {
    const std::string header = "The Project Gutenberg eBook of Robinson Crusoe\n"
                               "\n"
                               "Title: Robinson Crusoe\n"
                               "Author: Daniel Defoe\n"
                               "Release date: October 1, 1996 [eBook #521]\n"
                               "Language: English\n";

    const Book book = HeaderParser().book(BookText{5, header, "", BODY});

    EXPECT_EQ((Book{5, "Robinson Crusoe", "Daniel Defoe", "English", BODY}), book);
}

TEST(HeaderParserTest, leavesMissingFieldsEmpty) {
    const Book book = HeaderParser().book(BookText{5, "Title: Only a title", "", BODY});

    EXPECT_EQ("Only a title", book.title);
    EXPECT_EQ(std::nullopt, book.author);
    EXPECT_EQ(std::nullopt, book.language);
}

TEST(HeaderParserTest, takesABlankValueAsEmptyTextButAColonAloneAsNoField) {
    EXPECT_EQ("", HeaderParser::field("Author:   \nAuthor: Second", "Author"));
    EXPECT_EQ("Second", HeaderParser::field("Author:\nAuthor: Second", "Author"));
    EXPECT_EQ(std::nullopt, HeaderParser::field("Release Author: Someone", "Author"));
}

}
}
