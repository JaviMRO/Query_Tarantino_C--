#include "tarantino/indexer/adapters/datalake/batch/BatchBasedDatalakeReader.hpp"
#include "tarantino/indexer/adapters/datalake/book/BookBasedDatalakeReader.hpp"
#include "tarantino/indexer/adapters/datalake/time/TimeBasedDatalakeReader.hpp"
#include "tarantino/platform/Files.hpp"
#include "tarantino/testing/TempDirectory.hpp"

#include <gtest/gtest.h>

namespace tarantino::indexer::adapters {
namespace {

namespace files = platform::files;
using model::BookText;

class DatalakeReadersTest : public ::testing::Test {
protected:
    void store(std::string_view header, std::string_view body) {
        files::createDirectories((root / body).parent_path());
        files::writeString(root / header, "header");
        files::writeString(root / body, "body");
    }

    std::optional<BookText> book(int bookId, std::string_view body) {
        return BookText{bookId, "header", "body", root / body};
    }

    testing::TempDirectory root;
};

TEST_F(DatalakeReadersTest, readsEveryLayout) {
    store("20250925/14/5.header.txt", "20250925/14/5.body.txt");
    store("1342/header.txt", "1342/body.txt");
    store("64/64317.header.txt", "64/64317.body.txt");

    EXPECT_EQ(book(5, "20250925/14/5.body.txt"), TimeBasedDatalakeReader(root.path()).bookText(5));
    EXPECT_EQ(book(1342, "1342/body.txt"), BookBasedDatalakeReader(root.path()).bookText(1342));
    EXPECT_EQ(book(64317, "64/64317.body.txt"), BatchBasedDatalakeReader(root.path()).bookText(64317));
}

TEST_F(DatalakeReadersTest, ignoresBooksWithoutBody) {
    files::createDirectories(root / "7");
    files::writeString(root / "7/header.txt", "header");

    EXPECT_EQ(std::nullopt, BookBasedDatalakeReader(root.path()).bookText(7));
    EXPECT_EQ(std::nullopt, TimeBasedDatalakeReader(root / "missing").bookText(7));
}

}
}
