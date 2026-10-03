#pragma once

#include "tarantino/control/model/Outcome.hpp"

#include <map>
#include <vector>

namespace tarantino::control::ports {

/// Indexes books for the control layer (SPEC §9).
class Indexer {
public:
    virtual ~Indexer() = default;

    /// Indexes the books together, flushing the index once, and returns the outcome of each one by id.
    virtual std::map<int, model::Outcome> index(const std::vector<int>& bookIds) = 0;
};

}
