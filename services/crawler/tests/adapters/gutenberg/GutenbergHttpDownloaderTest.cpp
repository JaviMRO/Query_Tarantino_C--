#include "tarantino/crawler/adapters/gutenberg/GutenbergHttpDownloader.hpp"
#include "tarantino/crawler/model/failure/DownloadException.hpp"

#include "crawler/tests/adapters/gutenberg/TestHttpServer.hpp"

#include <gtest/gtest.h>

#include <deque>
#include <mutex>

namespace tarantino::crawler::adapters {
namespace {

using model::DownloadException;
using model::FailureReason;
using Answer = TestHttpServer::Answer;
using std::chrono::seconds;

const platform::Instant NOW = *platform::parseInstant("2026-10-01T12:00:00Z");
constexpr const char* TEXT = "Café *** START";

class GutenbergHttpDownloaderTest : public ::testing::Test {
protected:
    GutenbergHttpDownloaderTest()
        : mirror([this](const std::string& path) {
              const std::lock_guard lock(mutex);
              requests.push_back(path);
              if (answers.empty()) {
                  return Answer{200, {}, TEXT};
              }
              Answer answer = answers.front();
              answers.pop_front();
              return answer;
          }),
          downloader(downloaderOf("http://127.0.0.1:" + std::to_string(mirror.port()) + "/gutenberg-epub/%d/pg%d.txt")) {
    }

    GutenbergHttpDownloader downloaderOf(const std::string& urlTemplate) {
        return GutenbergHttpDownloader(urlTemplate, platform::fixedClock(NOW),
                                       [this](std::chrono::nanoseconds wait) { waits.push_back(wait); });
    }

    FailureReason failure(int bookId) {
        try {
            downloader.rawText(bookId);
        } catch (const DownloadException& error) {
            return error.reason();
        }
        ADD_FAILURE() << "book " << bookId << " was downloaded";
        return FailureReason::StorageError;
    }

    std::vector<std::string> recordedRequests() {
        const std::lock_guard lock(mutex);
        return requests;
    }

    std::mutex mutex;
    std::deque<Answer> answers;
    std::vector<std::string> requests;
    std::vector<std::chrono::nanoseconds> waits;
    TestHttpServer mirror;
    GutenbergHttpDownloader downloader;
};

TEST_F(GutenbergHttpDownloaderTest, returnsTheBodyDecodedAsUtf8) {
    EXPECT_EQ(TEXT, downloader.rawText(1342));
    EXPECT_EQ(std::vector<std::string>{"/gutenberg-epub/1342/pg1342.txt"}, recordedRequests());
}

TEST_F(GutenbergHttpDownloaderTest, replacesMalformedUtf8AsJavasDecoderDoes) {
    answers.push_back(Answer{200, {}, "caf\xE9"});

    EXPECT_EQ("caf�", downloader.rawText(1342));
}

TEST_F(GutenbergHttpDownloaderTest, mapsMissingBooksToNotFound) {
    answers.push_back(Answer{404, {}, ""});

    EXPECT_EQ(FailureReason::NotFound, failure(1342));
}

TEST_F(GutenbergHttpDownloaderTest, mapsServerErrorsToNetworkErrorWithoutRetrying) {
    answers.push_back(Answer{500, {}, ""});

    EXPECT_EQ(FailureReason::NetworkError, failure(1342));
    EXPECT_EQ(1u, recordedRequests().size());
}

TEST_F(GutenbergHttpDownloaderTest, mapsUnreachableServersToNetworkError) {
    GutenbergHttpDownloader unreachable = downloaderOf("http://localhost:1/%d/%d.txt");
    try {
        unreachable.rawText(1);
        FAIL() << "an unreachable server answered";
    } catch (const DownloadException& error) {
        EXPECT_EQ(FailureReason::NetworkError, error.reason());
    }
}

TEST_F(GutenbergHttpDownloaderTest, retriesABusyMirrorAfterTheWaitItAsksFor) {
    answers.push_back(Answer{429, {{"Retry-After", "120"}}, ""});

    EXPECT_EQ(TEXT, downloader.rawText(1342));
    EXPECT_EQ(std::vector<std::chrono::nanoseconds>{seconds(120)}, waits);
    EXPECT_EQ(2u, recordedRequests().size());
}

TEST_F(GutenbergHttpDownloaderTest, readsTheWaitAsAnHttpDate) {
    answers.push_back(Answer{503, {{"Retry-After", "Thu, 01 Oct 2026 12:01:30 GMT"}}, ""});

    EXPECT_EQ(TEXT, downloader.rawText(1342));
    EXPECT_EQ(std::vector<std::chrono::nanoseconds>{seconds(90)}, waits);
}

TEST_F(GutenbergHttpDownloaderTest, doublesTheWaitWhenTheMirrorDoesNotSayHowLong) {
    answers.push_back(Answer{503, {}, ""});
    answers.push_back(Answer{503, {}, ""});
    answers.push_back(Answer{429, {}, ""});

    EXPECT_EQ(TEXT, downloader.rawText(1342));
    EXPECT_EQ((std::vector<std::chrono::nanoseconds>{seconds(1), seconds(2), seconds(4)}), waits);
}

TEST_F(GutenbergHttpDownloaderTest, makesEveryDownloadWaitWhileTheMirrorIsBusy) {
    answers.push_back(Answer{429, {{"Retry-After", "60"}}, ""});

    downloader.rawText(1342);
    // The clock stands still, so the second download starts while the mirror still asks to wait
    downloader.rawText(84);

    EXPECT_EQ((std::vector<std::chrono::nanoseconds>{seconds(60), seconds(60)}), waits);
}

TEST_F(GutenbergHttpDownloaderTest, givesUpWhenTheMirrorStaysBusy) {
    for (int answer = 0; answer < 6; ++answer) {
        answers.push_back(Answer{429, {{"Retry-After", "0"}}, ""});
    }

    EXPECT_EQ(FailureReason::NetworkError, failure(1342));
    EXPECT_EQ(6u, recordedRequests().size());
}

TEST_F(GutenbergHttpDownloaderTest, givesUpWhenTheMirrorAsksToWaitTooLong) {
    answers.push_back(Answer{503, {{"Retry-After", "3600"}}, ""});

    EXPECT_EQ(FailureReason::NetworkError, failure(1342));
    EXPECT_TRUE(waits.empty());
    EXPECT_EQ(1u, recordedRequests().size());
}

TEST_F(GutenbergHttpDownloaderTest, followsRedirectsWithinTheMirror) {
    answers.push_back(Answer{302, {{"Location", "/moved/pg1342.txt"}}, ""});

    EXPECT_EQ(TEXT, downloader.rawText(1342));
    EXPECT_EQ((std::vector<std::string>{"/gutenberg-epub/1342/pg1342.txt", "/moved/pg1342.txt"}), recordedRequests());
}

TEST_F(GutenbergHttpDownloaderTest, givesUpAfterFiveRedirects) {
    for (int answer = 0; answer < 6; ++answer) {
        answers.push_back(Answer{301, {{"Location", "/again/" + std::to_string(answer)}}, ""});
    }

    EXPECT_EQ(FailureReason::NetworkError, failure(1342));
    EXPECT_EQ(6u, recordedRequests().size());
}

TEST_F(GutenbergHttpDownloaderTest, neverFollowsARedirectToAnotherHost) {
    std::vector<std::string> otherRequests;
    std::mutex otherMutex;
    TestHttpServer otherHost([&](const std::string& path) {
        const std::lock_guard lock(otherMutex);
        otherRequests.push_back(path);
        return Answer{200, {}, TEXT};
    });
    answers.push_back(Answer{301, {{"Location", "http://localhost:" + std::to_string(otherHost.port()) + "/cache/epub/1342/pg1342.txt"}}, ""});

    EXPECT_EQ(FailureReason::NetworkError, failure(1342));
    const std::lock_guard lock(otherMutex);
    EXPECT_TRUE(otherRequests.empty());
}

}
}
