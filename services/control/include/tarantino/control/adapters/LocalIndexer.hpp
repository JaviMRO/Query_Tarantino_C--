#pragma once

#include "tarantino/control/ports/Indexer.hpp"
#include "tarantino/indexer/commands/IndexBookCommand.hpp"

namespace tarantino::control::adapters {

/// Indexes batches with the indexer of this process, one index flush per batch (SPEC §9).
class LocalIndexer : public ports::Indexer {
public:
    explicit LocalIndexer(indexer::commands::IndexBookCommand index);

    std::map<int, model::Outcome> index(const std::vector<int>& bookIds) override;

private:
    indexer::commands::IndexBookCommand index_;
};

}
