#pragma once

#include "tarantino/control/model/NextStep.hpp"
#include "tarantino/control/model/Outcome.hpp"

#include <string>

namespace tarantino::control::model {

/// A step the control layer took and how it went.
struct StepReport {
    NextStep step;
    Outcome outcome;

    bool idle() const {
        return step.action == NextStep::Action::Idle;
    }

    /// <ACTION> <books>: <detail>, what the control service prints after [CONTROL] (SPEC §15).
    std::string description() const {
        return std::string(name(step.action)) + " " + step.books() + ": " + outcome.detail;
    }

    bool operator==(const StepReport&) const = default;
};

}
