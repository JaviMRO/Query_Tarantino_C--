#include "tarantino/control/ControlConfig.hpp"

#include "tarantino/platform/Environment.hpp"
#include "tarantino/platform/Paths.hpp"
#include "tarantino/platform/Text.hpp"

#include <stdexcept>

namespace tarantino::control {

namespace {

constexpr const char* PARALLEL_DOWNLOADS = "TARANTINO_PARALLEL_DOWNLOADS";
constexpr const char* INDEX_BATCH = "TARANTINO_INDEX_BATCH";

}

ControlConfig ControlConfig::fromEnvironment() {
    using platform::environmentVariable;
    return {platform::pathOf(environmentVariable("TARANTINO_CONTROL", "control")),
            platform::pathOf(environmentVariable("TARANTINO_WORKLOAD", "workload")),
            positiveInteger(PARALLEL_DOWNLOADS, environmentVariable(PARALLEL_DOWNLOADS, "8")),
            positiveInteger(INDEX_BATCH, environmentVariable(INDEX_BATCH, "100"))};
}

int ControlConfig::positiveInteger(const std::string& name, const std::string& value) {
    std::string_view number = platform::strip(value);
    if (number.starts_with('+')) {
        number.remove_prefix(1);
    }
    const auto parsed = platform::parseDigits(number);
    if (!parsed || *parsed <= 0) {
        throw std::invalid_argument("Unknown " + name + ": " + value + " (expected a positive integer)");
    }
    return *parsed;
}

}
