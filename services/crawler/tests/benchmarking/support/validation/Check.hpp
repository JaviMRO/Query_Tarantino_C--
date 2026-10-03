#pragma once

#include <stdexcept>
#include <string>

namespace tarantino::crawler::benchmarking {

/// What every benchmark checks before measuring (SPEC §11): a structure that gives a wrong result fails the run, which
/// then writes no results.
class Check {
public:
    Check() = delete;

    static void require(bool condition, const std::string& failure) {
        if (!condition) {
            throw std::runtime_error("Invalid benchmark result: " + failure);
        }
    }
};

}
