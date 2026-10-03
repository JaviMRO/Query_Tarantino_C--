#include "tarantino/crawler/adapters/gutenberg/LocalMirrorDownloader.hpp"
#include "tarantino/crawler/model/failure/DownloadException.hpp"
#include "tarantino/platform/Files.hpp"
#include "tarantino/testing/TempDirectory.hpp"

#include <gtest/gtest.h>

namespace tarantino::crawler::adapters {
namespace {

namespace files = platform::files;
using model::DownloadException;
using model::FailureReason;

class LocalMirrorDownloaderTest : public ::testing::Test {
protected:
    void write(std::string_view file, std::string_view text) {
        files::createDirectories((mirror / file).parent_path());
        files::writeString(mirror / file, text);
    }

    testing::TempDirectory mirror;
};

TEST_F(LocalMirrorDownloaderTest, readsABookFromTheGeneratedCollectionLayout) {
    write("1342/pg1342.txt", "Title: Pride and Prejudice — café");

    EXPECT_EQ("Title: Pride and Prejudice — café", LocalMirrorDownloader(mirror.path()).rawText(1342));
}

TEST_F(LocalMirrorDownloaderTest, reportsABookMissingFromTheMirrorAsNotFound) {
    try {
        LocalMirrorDownloader(mirror.path()).rawText(84);
        FAIL() << "a missing book was read";
    } catch (const DownloadException& error) {
        EXPECT_EQ(FailureReason::NotFound, error.reason());
    }
}

TEST_F(LocalMirrorDownloaderTest, reportsATextThatIsNotUtf8AsANetworkErrorAsJavaReadsIt) {
    write("84/pg84.txt", "caf\xE9");
    try {
        LocalMirrorDownloader(mirror.path()).rawText(84);
        FAIL() << "a text that is not UTF-8 was read";
    } catch (const DownloadException& error) {
        EXPECT_EQ(FailureReason::NetworkError, error.reason());
    }
}

TEST_F(LocalMirrorDownloaderTest, listsTheBooksWithATextInIdOrder) {
    for (const std::string_view file : {"1342/pg1342.txt", "84/pg84.txt", "11/pg11-images.epub", "README", "cache/pgcache.txt", "007/pg7.txt"}) {
        write(file, "text");
    }

    EXPECT_EQ((std::vector<int>{84, 1342}), LocalMirrorDownloader(mirror.path()).bookIds());
}

}
}
