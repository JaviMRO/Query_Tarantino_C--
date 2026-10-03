#include "tarantino/crawler/commands/IngestBookCommand.hpp"
#include "tarantino/crawler/model/failure/DownloadException.hpp"

#include "crawler/tests/ports/FunctionDownloader.hpp"

#include <gtest/gtest.h>

#include <map>
#include <system_error>

namespace tarantino::crawler::commands {
namespace {

using model::BookText;
using model::DownloadException;
using model::FailureReason;
using model::StoredPaths;
using ports::FunctionDownloader;

constexpr const char* RAW = "Title: T\n*** START OF THE PROJECT GUTENBERG EBOOK T ***\nbody\n*** END OF THE PROJECT GUTENBERG EBOOK T ***";

class InMemoryDatalake : public ports::DatalakeStorage {
public:
    static StoredPaths paths(int bookId) {
        return {std::to_string(bookId) + ".header.txt", std::to_string(bookId) + ".body.txt"};
    }

    StoredPaths save(const BookText& book) override {
        books.insert_or_assign(book.bookId, book);
        return paths(book.bookId);
    }

    std::optional<StoredPaths> pathsOf(int bookId) override {
        return books.contains(bookId) ? std::optional(paths(bookId)) : std::nullopt;
    }

    std::unordered_set<int> idsStoredSince(platform::Instant) override {
        std::unordered_set<int> ids;
        for (const auto& [bookId, book] : books) {
            ids.insert(bookId);
        }
        return ids;
    }

    int removeIncompleteWrites() override {
        return 0;
    }

    std::map<int, BookText> books;
};

const auto UNREACHABLE = FunctionDownloader::of([](int bookId) -> std::string {
    ADD_FAILURE() << "Book " << bookId << " must not be downloaded";
    return "";
});

class IngestBookCommandTest : public ::testing::Test {
protected:
    std::shared_ptr<InMemoryDatalake> datalake = std::make_shared<InMemoryDatalake>();
};

TEST_F(IngestBookCommandTest, downloadsSplitsAndStoresNewBooks) {
    const IngestResult result = IngestBookCommand(FunctionDownloader::of([](int) { return RAW; }), datalake).execute(5);

    EXPECT_EQ(IngestResult::success(5, InMemoryDatalake::paths(5)), result);
    EXPECT_EQ((BookText{5, "Title: T", "body"}), datalake->books.at(5));
}

TEST_F(IngestBookCommandTest, reusesBooksAlreadyInTheDatalakeWithoutDownloading) {
    datalake->save(BookText{5, "header", "body"});

    EXPECT_EQ(IngestResult::success(5, InMemoryDatalake::paths(5)), IngestBookCommand(UNREACHABLE, datalake).execute(5));
}

TEST_F(IngestBookCommandTest, reportsDownloadFailuresWithoutStoring) {
    const auto missing = FunctionDownloader::of([](int) -> std::string {
        throw DownloadException(FailureReason::NotFound, "missing");
    });

    EXPECT_EQ(IngestResult::failed(5, FailureReason::NotFound), IngestBookCommand(missing, datalake).execute(5));
    EXPECT_FALSE(datalake->books.contains(5));
}

TEST_F(IngestBookCommandTest, reportsBooksWithoutMarkersAsMissingMarkers) {
    EXPECT_EQ(IngestResult::failed(5, FailureReason::MissingMarkers),
              IngestBookCommand(FunctionDownloader::of([](int) { return "no markers"; }), datalake).execute(5));
}

TEST_F(IngestBookCommandTest, downloadsWithoutWritingTheDatalakeUntilTheBookIsStored) {
    IngestBookCommand ingest(FunctionDownloader::of([](int) { return RAW; }), datalake);

    const DownloadResult download = ingest.download(5);
    EXPECT_EQ(DownloadResult::success(BookText{5, "Title: T", "body"}), download);
    EXPECT_FALSE(datalake->books.contains(5));

    EXPECT_EQ(IngestResult::success(5, InMemoryDatalake::paths(5)), ingest.store(download));
    EXPECT_EQ((BookText{5, "Title: T", "body"}), datalake->books.at(5));
}

TEST_F(IngestBookCommandTest, storesNothingForAFailedDownload) {
    IngestBookCommand ingest(UNREACHABLE, datalake);

    EXPECT_EQ(IngestResult::failed(5, FailureReason::NetworkError), ingest.store(DownloadResult::failed(5, FailureReason::NetworkError)));
    EXPECT_FALSE(datalake->books.contains(5));
}

TEST_F(IngestBookCommandTest, reportsStorageErrors) {
    class BrokenDatalake : public InMemoryDatalake {
    public:
        StoredPaths save(const BookText&) override {
            throw std::filesystem::filesystem_error("disk full", std::make_error_code(std::errc::no_space_on_device));
        }
    };

    EXPECT_EQ(IngestResult::failed(5, FailureReason::StorageError),
              IngestBookCommand(FunctionDownloader::of([](int) { return RAW; }), std::make_shared<BrokenDatalake>()).execute(5));
}

}
}
