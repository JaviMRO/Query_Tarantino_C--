#pragma once

#include <filesystem>
#include <string>

namespace tarantino::control {

/// The configuration of the control service (SPEC §2): where its files are, how many books it downloads at once (D)
/// and how many it indexes per index flush (K).
struct ControlConfig {
    std::filesystem::path control;
    std::filesystem::path workload;
    int parallelDownloads;
    int indexBatch;

    static ControlConfig fromEnvironment();

    /// The value as a positive integer, or std::invalid_argument naming the variable and the value.
    static int positiveInteger(const std::string& name, const std::string& value);
};

}
