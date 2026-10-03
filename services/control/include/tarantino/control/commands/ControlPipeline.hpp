#pragma once

#include "tarantino/control/model/StepReport.hpp"
#include "tarantino/control/ports/ControlStateStore.hpp"
#include "tarantino/control/ports/Crawler.hpp"
#include "tarantino/control/ports/Indexer.hpp"
#include "tarantino/platform/BlockingQueue.hpp"

#include <deque>
#include <map>
#include <memory>
#include <optional>
#include <unordered_set>
#include <vector>

namespace tarantino::control::commands {

/// The control layer of SPEC §9. It reads the state once and keeps it in memory, so each step costs the same however
/// many books are done. Up to parallelDownloads books are downloaded at once, started in candidates order; whichever
/// finishes first is stored, marked as downloaded and queued for indexing by the pipeline's own thread, the only one
/// that uses the datalake, the state and the index. Books are indexed in batches, one index flush per batch, and
/// marked as indexed only after it, while the next downloads go on.
class ControlPipeline {
public:
    ControlPipeline(std::shared_ptr<ports::ControlStateStore> state, std::shared_ptr<ports::Crawler> crawler,
                    std::shared_ptr<ports::Indexer> indexer, std::vector<int> candidates, int parallelDownloads,
                    int indexBatch);

    /// Starts the downloads it can, then indexes a batch if one is due, or else stores the first download to finish,
    /// or else reports that the run is over. An unexpected error of a download or of the indexer is thrown.
    model::StepReport runStep();

private:
    /// A download that finished in another thread, to be stored in the pipeline's.
    struct FinishedDownload {
        int bookId;
        ports::Crawler::Download download;
        std::exception_ptr error;
    };

    void startDownloads();

    void start(int bookId);

    std::optional<int> pendingCandidate();

    /// Downloaded in this run or a previous one, or started in this run, which a failed download is too.
    bool isDone(int bookId) const;

    model::Outcome downloaded(FinishedDownload download);

    model::Outcome indexed(const std::vector<int>& bookIds, const std::map<int, model::Outcome>& outcomes);

    std::shared_ptr<ports::ControlStateStore> state_;
    std::shared_ptr<ports::Crawler> crawler_;
    std::shared_ptr<ports::Indexer> indexer_;
    std::vector<int> candidates_;
    std::size_t parallelDownloads_;
    std::size_t indexBatch_;
    std::unordered_set<int> downloaded_;
    std::unordered_set<int> started_;
    std::deque<int> toIndex_;
    // Shared with the downloads, which may still finish after the pipeline is gone
    std::shared_ptr<platform::BlockingQueue<FinishedDownload>> finished_;
    std::size_t running_ = 0;
    std::size_t nextCandidate_ = 0;
};

}
