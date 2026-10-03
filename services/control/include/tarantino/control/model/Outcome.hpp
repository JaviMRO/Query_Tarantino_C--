#pragma once

#include <ostream>
#include <string>

namespace tarantino::control::model {

/// How a step went for a book or a batch, in the words of the crawler or indexer line (SPEC §15).
struct Outcome {
    bool succeeded;
    std::string detail;

    static Outcome success(std::string detail) {
        return {true, std::move(detail)};
    }

    static Outcome failure(std::string detail) {
        return {false, std::move(detail)};
    }

    bool operator==(const Outcome&) const = default;
};

inline std::ostream& operator<<(std::ostream& out, const Outcome& outcome) {
    return out << (outcome.succeeded ? "success(" : "failure(") << outcome.detail << ")";
}

}
