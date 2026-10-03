#include "tarantino/control/adapters/LocalCrawler.hpp"
#include "tarantino/control/adapters/LocalIndexer.hpp"
#include "tarantino/crawler/model/failure/DownloadException.hpp"

#include "crawler/tests/ports/FunctionDownloader.hpp"

#include <gtest/gtest.h>

#include <map>

namespace tarantino::control::adapters {
namespace {

using crawler::model::DownloadException;
using crawler::model::FailureReason;
using crawler::model::StoredPaths;
using crawler::ports::FunctionDownloader;
using model::Outcome;

constexpr const char* RAW = "Title: T\n*** START OF THE PROJECT GUTENBERG EBOOK T ***\nbody\n*** END OF THE PROJECT GUTENBERG EBOOK T ***";

class RecordingDatalake : public crawler::ports::DatalakeStorage {
public:
    StoredPaths save(const crawler::model::BookText& book) override {
        const std::filesystem::path directory = std::filesystem::path("datalake") / std::to_string(book.bookId);
        const StoredPaths paths{directory / "header.txt", directory / "body.txt"};
        books.insert_or_assign(book.bookId, paths);
        saved.push_back(book.bookId);
        return paths;
    }

    std::optional<StoredPaths> pathsOf(int bookId) override {
        const auto found = books.find(bookId);
        return found == books.end() ? std::nullopt : std::optional(found->second);
    }

    std::unordered_set<int> idsStoredSince(platform::Instant) override {
        return {};
    }

    int removeIncompleteWrites() override {
        return 0;
    }

    std::map<int, StoredPaths> books;
    std::vector<int> saved;
};

const auto UNREACHABLE = FunctionDownloader::of([](int bookId) -> std::string {
    ADD_FAILURE() << "Book " << bookId << " must not be downloaded";
    return "";
});

/// The outcome of the download the crawler reports, stored as the pipeline stores it.
struct Finished {
    ports::Crawler::Download download;
    std::exception_ptr error;
    bool done = false;
};

LocalCrawler::Executor runAtOnce() {
    return [](std::function<void()> task) { task(); };
}

class LocalAdaptersTest : public ::testing::Test {
protected:
    std::shared_ptr<RecordingDatalake> datalake = std::make_shared<RecordingDatalake>();
};

TEST_F(LocalAdaptersTest, crawlerReportsWhereABookAlreadyStoredIsWithoutDownloadingIt) {
    datalake->save(crawler::model::BookText{5, "header", "body"});
    LocalCrawler crawler(crawler::commands::IngestBookCommand(UNREACHABLE, datalake), runAtOnce());
    Finished finished;

    crawler.ingest(5, [&](ports::Crawler::Download download, std::exception_ptr error) {
        finished = Finished{std::move(download), error, true};
    });

    ASSERT_TRUE(finished.done);
    EXPECT_EQ(Outcome::success("stored in datalake/5"), finished.download());
}

TEST_F(LocalAdaptersTest, crawlerDownloadsWithItsExecutorAndWritesTheDatalakeOnlyWhenStoring) {
    std::vector<std::function<void()>> downloads;
    LocalCrawler crawler(crawler::commands::IngestBookCommand(FunctionDownloader::of([](int) { return RAW; }), datalake),
                         [&](std::function<void()> task) { downloads.push_back(std::move(task)); });
    Finished finished;

    crawler.ingest(5, [&](ports::Crawler::Download download, std::exception_ptr error) {
        finished = Finished{std::move(download), error, true};
    });
    EXPECT_FALSE(finished.done);
    for (const auto& download : downloads) {
        download();
    }
    ASSERT_TRUE(finished.done);
    EXPECT_TRUE(datalake->saved.empty());

    EXPECT_EQ(Outcome::success("stored in datalake/5"), finished.download());
    EXPECT_EQ(std::vector<int>{5}, datalake->saved);
}

TEST_F(LocalAdaptersTest, crawlerReportsTheFailureReason) {
    const auto missing = FunctionDownloader::of([](int) -> std::string {
        throw DownloadException(FailureReason::NotFound, "missing");
    });
    LocalCrawler crawler(crawler::commands::IngestBookCommand(missing, datalake), runAtOnce());
    Finished finished;

    crawler.ingest(5, [&](ports::Crawler::Download download, std::exception_ptr error) {
        finished = Finished{std::move(download), error, true};
    });

    EXPECT_EQ(Outcome::failure("skipped, NOT_FOUND"), finished.download());
    EXPECT_TRUE(datalake->saved.empty());
}

TEST_F(LocalAdaptersTest, crawlerPassesOnAnUnexpectedErrorOfTheDownload) {
    const auto broken = FunctionDownloader::of([](int) -> std::string { throw std::logic_error("bug in the downloader"); });
    LocalCrawler crawler(crawler::commands::IngestBookCommand(broken, datalake), runAtOnce());
    Finished finished;

    crawler.ingest(5, [&](ports::Crawler::Download download, std::exception_ptr error) {
        finished = Finished{std::move(download), error, true};
    });

    ASSERT_TRUE(finished.error);
    EXPECT_THROW(std::rethrow_exception(finished.error), std::logic_error);
}

class FunctionDatalake : public indexer::ports::DatalakeReader {
public:
    std::optional<indexer::model::BookText> bookText(int bookId) override {
        if (bookId != 5) {
            return std::nullopt;
        }
        return indexer::model::BookText{5, "Title: T", "island whale island", "5.body.txt"};
    }
};

class NoIndex : public indexer::ports::InvertedIndexStorage {
public:
    void add(const indexer::model::TermOccurrences&) override {
    }

    void flush() override {
    }
};

class NoMetadata : public indexer::ports::MetadataStorage {
public:
    void save(const indexer::model::Book&) override {
    }
};

TEST(LocalIndexerTest, reportsTheUniqueTermsOfEachBookAndTheOnesMissingFromTheDatalake) {
    LocalIndexer indexer(indexer::commands::IndexBookCommand(std::make_shared<FunctionDatalake>(), indexer::model::HeaderParser(),
                                                             indexer::model::Tokenizer(platform::StringSet{}),
                                                             std::make_shared<NoIndex>(), std::make_shared<NoMetadata>()));

    EXPECT_EQ((std::map<int, Outcome>{{5, Outcome::success("2 unique terms indexed")},
                                      {6, Outcome::failure("skipped, not found in the datalake")}}),
              indexer.index({5, 6}));
}

}
}
