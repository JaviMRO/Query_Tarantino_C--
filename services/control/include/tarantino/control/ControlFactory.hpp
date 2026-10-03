#pragma once

#include "tarantino/control/ControlConfig.hpp"
#include "tarantino/control/commands/ControlPipeline.hpp"
#include "tarantino/crawler/CrawlerConfig.hpp"
#include "tarantino/indexer/IndexerConfig.hpp"

#include <memory>
#include <optional>
#include <ostream>
#include <string>
#include <vector>

namespace tarantino::control {

class ControlFactory {
public:
    ControlFactory() = delete;

    /// The pipeline of the control service, with the crawler and indexer of this process and a pool of D threads for
    /// the downloads, once what an interrupted run left in the datalake is removed (SPEC §6) and reported on out.
    static std::unique_ptr<commands::ControlPipeline> pipeline(const ControlConfig& config, const crawler::CrawlerConfig& crawler,
                                                               const indexer::IndexerConfig& indexer,
                                                               std::vector<int> candidates, std::ostream& out);

    /// The ids of the workload file given, in order. Without one, every book of the local mirror in ascending id order
    /// when there is a mirror, or else the sample dataset (SPEC §9).
    static std::vector<int> candidates(const ControlConfig& config, const crawler::CrawlerConfig& crawler,
                                       const std::optional<std::string>& file);
};

}
