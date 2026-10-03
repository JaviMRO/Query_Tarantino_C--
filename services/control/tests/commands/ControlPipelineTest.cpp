#include "tarantino/control/commands/ControlPipeline.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <set>

namespace tarantino::control::commands {
namespace {

using model::NextStep;
using model::Outcome;
using model::StepReport;

constexpr int MISSING_BOOK = 404;
constexpr int ONE_AT_A_TIME = 1;
constexpr int BOOK_BY_BOOK = 1;
constexpr int DEFAULT_BATCH = 100;

class InMemoryState : public ports::ControlStateStore {
public:
    std::vector<int> downloaded() override {
        ++reads;
        return downloadedIds;
    }

    std::vector<int> indexed() override {
        return indexedIds;
    }

    void markDownloaded(int bookId) override {
        if (std::find(downloadedIds.begin(), downloadedIds.end(), bookId) == downloadedIds.end()) {
            downloadedIds.push_back(bookId);
        }
    }

    void markIndexed(int bookId) override {
        if (std::find(indexedIds.begin(), indexedIds.end(), bookId) == indexedIds.end()) {
            indexedIds.push_back(bookId);
        }
    }

    std::set<int> indexedSet() const {
        return {indexedIds.begin(), indexedIds.end()};
    }

    std::vector<int> downloadedIds;
    std::vector<int> indexedIds;
    int reads = 0;
};

/// Downloads that finish at once, in the order they start; a download runs until the pipeline stores it.
class RecordingCrawler : public ports::Crawler {
public:
    void ingest(int bookId, WhenFinished whenFinished) override {
        started.push_back(bookId);
        mostRunning = std::max(mostRunning, static_cast<int>(started.size()) - stored);
        whenFinished([this, bookId] {
            ++stored;
            return bookId == MISSING_BOOK ? Outcome::failure("not found") : Outcome::success("stored");
        }, nullptr);
    }

    std::vector<int> started;
    int stored = 0;
    int mostRunning = 0;
};

class RecordingIndexer : public ports::Indexer {
public:
    std::map<int, Outcome> index(const std::vector<int>& bookIds) override {
        batches.push_back(bookIds);
        std::map<int, Outcome> outcomes;
        for (const int bookId : bookIds) {
            outcomes.emplace(bookId, bookId == MISSING_BOOK ? Outcome::failure("not found") : Outcome::success("indexed"));
        }
        return outcomes;
    }

    std::vector<std::vector<int>> batches;
};

/// A crawler or indexer made of a function, as a Java lambda implements one.
class FunctionCrawler : public ports::Crawler {
public:
    explicit FunctionCrawler(std::function<void(int, WhenFinished)> ingest) : ingest_(std::move(ingest)) {
    }

    void ingest(int bookId, WhenFinished whenFinished) override {
        ingest_(bookId, std::move(whenFinished));
    }

private:
    std::function<void(int, WhenFinished)> ingest_;
};

class FunctionIndexer : public ports::Indexer {
public:
    explicit FunctionIndexer(std::function<std::map<int, Outcome>(const std::vector<int>&)> index) : index_(std::move(index)) {
    }

    std::map<int, Outcome> index(const std::vector<int>& bookIds) override {
        return index_(bookIds);
    }

private:
    std::function<std::map<int, Outcome>(const std::vector<int>&)> index_;
};

class ControlPipelineTest : public ::testing::Test {
protected:
    ControlPipeline pipeline(std::vector<int> candidates, int parallelDownloads, int indexBatch) {
        return ControlPipeline(state, crawler, indexer, std::move(candidates), parallelDownloads, indexBatch);
    }

    static std::vector<NextStep> steps(ControlPipeline& pipeline) {
        std::vector<NextStep> taken;
        for (StepReport report = pipeline.runStep(); !report.idle(); report = pipeline.runStep()) {
            taken.push_back(report.step);
        }
        return taken;
    }

    std::shared_ptr<InMemoryState> state = std::make_shared<InMemoryState>();
    std::shared_ptr<RecordingCrawler> crawler = std::make_shared<RecordingCrawler>();
    std::shared_ptr<RecordingIndexer> indexer = std::make_shared<RecordingIndexer>();
};

TEST_F(ControlPipelineTest, downloadingOneAtATimeDownloadsThenIndexesEachCandidateAndSkipsFailures) {
    ControlPipeline control = pipeline({1, MISSING_BOOK, 2}, ONE_AT_A_TIME, BOOK_BY_BOOK);

    EXPECT_EQ((std::vector<NextStep>{NextStep::download(1), NextStep::index({1}), NextStep::download(MISSING_BOOK),
                                     NextStep::download(2), NextStep::index({2})}),
              steps(control));
    EXPECT_EQ((std::set<int>{1, 2}), state->indexedSet());
}

TEST_F(ControlPipelineTest, indexesInBatchesAndTheLastPartialBatchWhenNothingIsLeftToDownload) {
    ControlPipeline control = pipeline({1, 2, 3}, ONE_AT_A_TIME, 2);

    EXPECT_EQ((std::vector<NextStep>{NextStep::download(1), NextStep::download(2), NextStep::index({1, 2}),
                                     NextStep::download(3), NextStep::index({3})}),
              steps(control));
    EXPECT_EQ((std::vector<std::vector<int>>{{1, 2}, {3}}), indexer->batches);
    EXPECT_EQ((std::set<int>{1, 2, 3}), state->indexedSet());
}

TEST_F(ControlPipelineTest, downloadsUpToTheParallelDownloadsAtOnceStartedInCandidatesOrder) {
    ControlPipeline control = pipeline({1, 2, 3, 4, 5}, 3, DEFAULT_BATCH);

    control.runStep();
    EXPECT_EQ((std::vector<int>{1, 2, 3}), crawler->started);

    steps(control);
    EXPECT_EQ((std::vector<int>{1, 2, 3, 4, 5}), crawler->started);
    EXPECT_EQ(3, crawler->mostRunning);
    EXPECT_EQ((std::set<int>{1, 2, 3, 4, 5}), state->indexedSet());
}

TEST_F(ControlPipelineTest, storesMarksAndQueuesBooksInTheOrderTheirDownloadsFinish) {
    ports::Crawler::WhenFinished slowDownload;
    const auto firstFinishesLast = std::make_shared<FunctionCrawler>([&](int bookId, ports::Crawler::WhenFinished whenFinished) {
        if (bookId == 1) {
            slowDownload = std::move(whenFinished);
            return;
        }
        whenFinished([&] {
            slowDownload([] { return Outcome::success("stored"); }, nullptr);
            return Outcome::success("stored");
        }, nullptr);
    });
    ControlPipeline control(state, firstFinishesLast, indexer, {1, 2}, 2, 2);

    EXPECT_EQ((std::vector<NextStep>{NextStep::download(2), NextStep::download(1), NextStep::index({2, 1})}), steps(control));
    EXPECT_EQ((std::vector<int>{2, 1}), state->downloadedIds);
}

TEST_F(ControlPipelineTest, startsTheNextDownloadsBeforeIndexingABatch) {
    std::vector<std::vector<int>> startedWhenIndexing;
    const auto watched = std::make_shared<FunctionIndexer>([&](const std::vector<int>& bookIds) {
        startedWhenIndexing.push_back(crawler->started);
        return indexer->index(bookIds);
    });
    ControlPipeline control(state, crawler, watched, {1, 2, 3}, 2, BOOK_BY_BOOK);

    steps(control);

    EXPECT_EQ((std::vector<int>{1, 2, 3}), startedWhenIndexing.front());
}

TEST_F(ControlPipelineTest, stopsWhenADownloadFailsUnexpectedly) {
    const auto broken = std::make_shared<FunctionCrawler>([](int, ports::Crawler::WhenFinished whenFinished) {
        whenFinished(nullptr, std::make_exception_ptr(std::logic_error("bug in the crawler")));
    });
    ControlPipeline control(state, broken, indexer, {1}, ONE_AT_A_TIME, BOOK_BY_BOOK);

    EXPECT_THROW(control.runStep(), std::logic_error);
    EXPECT_TRUE(state->downloadedIds.empty());
}

TEST_F(ControlPipelineTest, resumesByIndexingBooksDownloadedBeforeAnInterruption) {
    state->markDownloaded(7);

    ControlPipeline control = pipeline({}, ONE_AT_A_TIME, DEFAULT_BATCH);

    EXPECT_EQ(NextStep::index({7}), control.runStep().step);
}

TEST_F(ControlPipelineTest, marksTheBooksOfABatchThatSucceedAndGoesOnWithoutTheOnesThatFail) {
    state->markDownloaded(MISSING_BOOK);
    state->markDownloaded(8);
    ControlPipeline control = pipeline({}, ONE_AT_A_TIME, DEFAULT_BATCH);

    const StepReport report = control.runStep();

    EXPECT_EQ(Outcome::failure("1 indexed, 1 skipped"), report.outcome);
    EXPECT_EQ(std::set<int>{8}, state->indexedSet());
    EXPECT_TRUE(control.runStep().idle());
}

TEST_F(ControlPipelineTest, marksNoBookOfABatchInterruptedBeforeItsFlush) {
    state->markDownloaded(1);
    state->markDownloaded(2);
    const auto interrupted = std::make_shared<FunctionIndexer>([](const std::vector<int>&) -> std::map<int, Outcome> {
        throw std::runtime_error("interrupted while flushing");
    });
    ControlPipeline control(state, crawler, interrupted, {}, ONE_AT_A_TIME, DEFAULT_BATCH);

    EXPECT_THROW(control.runStep(), std::runtime_error);
    EXPECT_TRUE(state->indexedIds.empty());
}

TEST_F(ControlPipelineTest, skipsCandidatesAlreadyDownloadedOrRepeated) {
    state->markDownloaded(1);
    state->markIndexed(1);
    ControlPipeline control = pipeline({1, 2, 1, 2}, 2, BOOK_BY_BOOK);

    EXPECT_EQ((std::vector<NextStep>{NextStep::download(2), NextStep::index({2})}), steps(control));
    EXPECT_EQ(std::vector<int>{2}, crawler->started);
}

TEST_F(ControlPipelineTest, readsTheStateOnceWhateverTheNumberOfSteps) {
    ControlPipeline control = pipeline({1, 2, 3, 4, 5}, ONE_AT_A_TIME, BOOK_BY_BOOK);

    steps(control);

    EXPECT_EQ(1, state->reads);
}

}
}
