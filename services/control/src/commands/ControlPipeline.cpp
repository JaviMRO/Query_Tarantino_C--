#include "tarantino/control/commands/ControlPipeline.hpp"

#include <algorithm>

namespace tarantino::control::commands {

using model::NextStep;
using model::Outcome;
using model::StepReport;

namespace {

const Outcome MISSING_OUTCOME = Outcome::failure("no outcome from the indexer");

const Outcome& outcomeOf(int bookId, const std::map<int, Outcome>& outcomes) {
    const auto found = outcomes.find(bookId);
    return found == outcomes.end() ? MISSING_OUTCOME : found->second;
}

Outcome summary(const std::vector<int>& bookIds, const std::map<int, Outcome>& outcomes) {
    const auto indexed = static_cast<std::size_t>(std::count_if(
            bookIds.begin(), bookIds.end(), [&](int bookId) { return outcomeOf(bookId, outcomes).succeeded; }));
    if (indexed == bookIds.size()) {
        return Outcome::success(std::to_string(indexed) + " indexed");
    }
    return Outcome::failure(std::to_string(indexed) + " indexed, " + std::to_string(bookIds.size() - indexed) + " skipped");
}

}

ControlPipeline::ControlPipeline(std::shared_ptr<ports::ControlStateStore> state, std::shared_ptr<ports::Crawler> crawler,
                                 std::shared_ptr<ports::Indexer> indexer, std::vector<int> candidates,
                                 int parallelDownloads, int indexBatch)
    : state_(std::move(state)),
      crawler_(std::move(crawler)),
      indexer_(std::move(indexer)),
      candidates_(std::move(candidates)),
      parallelDownloads_(static_cast<std::size_t>(parallelDownloads)),
      indexBatch_(static_cast<std::size_t>(indexBatch)),
      finished_(std::make_shared<platform::BlockingQueue<FinishedDownload>>()) {
    const std::vector<int> downloaded = state_->downloaded();
    const std::vector<int> indexedList = state_->indexed();
    const std::unordered_set<int> indexed(indexedList.begin(), indexedList.end());
    for (const int bookId : downloaded) {
        downloaded_.insert(bookId);
        if (!indexed.contains(bookId)) {
            toIndex_.push_back(bookId);
        }
    }
}

StepReport ControlPipeline::runStep() {
    startDownloads();
    if (toIndex_.size() >= indexBatch_ || (!toIndex_.empty() && running_ == 0)) {
        const std::vector<int> batch(toIndex_.begin(), toIndex_.begin() + static_cast<std::ptrdiff_t>(std::min(indexBatch_, toIndex_.size())));
        const std::map<int, Outcome> outcomes = indexer_->index(batch);
        return StepReport{NextStep::index(batch), indexed(batch, outcomes)};
    }
    if (running_ > 0) {
        FinishedDownload download = finished_->take();
        const int bookId = download.bookId;
        Outcome outcome = downloaded(std::move(download));
        return StepReport{NextStep::download(bookId), std::move(outcome)};
    }
    return StepReport{NextStep::idle(), Outcome::success("nothing left to do")};
}

void ControlPipeline::startDownloads() {
    while (running_ < parallelDownloads_) {
        const std::optional<int> candidate = pendingCandidate();
        if (!candidate) {
            return;
        }
        start(*candidate);
    }
}

void ControlPipeline::start(int bookId) {
    crawler_->ingest(bookId, [finished = finished_, bookId](ports::Crawler::Download download, std::exception_ptr error) {
        finished->add(FinishedDownload{bookId, std::move(download), std::move(error)});
    });
    started_.insert(bookId);
    ++running_;
}

std::optional<int> ControlPipeline::pendingCandidate() {
    while (nextCandidate_ < candidates_.size() && isDone(candidates_[nextCandidate_])) {
        ++nextCandidate_;
    }
    if (nextCandidate_ < candidates_.size()) {
        return candidates_[nextCandidate_];
    }
    return std::nullopt;
}

bool ControlPipeline::isDone(int bookId) const {
    return downloaded_.contains(bookId) || started_.contains(bookId);
}

Outcome ControlPipeline::downloaded(FinishedDownload download) {
    --running_;
    if (download.error) {
        std::rethrow_exception(download.error);
    }
    Outcome outcome = download.download();
    if (outcome.succeeded) {
        state_->markDownloaded(download.bookId);
        downloaded_.insert(download.bookId);
        toIndex_.push_back(download.bookId);
    }
    return outcome;
}

Outcome ControlPipeline::indexed(const std::vector<int>& bookIds, const std::map<int, Outcome>& outcomes) {
    for (const int bookId : bookIds) {
        toIndex_.pop_front();
        if (outcomeOf(bookId, outcomes).succeeded) {
            state_->markIndexed(bookId);
        }
    }
    return bookIds.size() == 1 ? outcomeOf(bookIds.front(), outcomes) : summary(bookIds, outcomes);
}

}
