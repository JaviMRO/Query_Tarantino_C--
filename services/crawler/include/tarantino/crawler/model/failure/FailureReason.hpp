#pragma once

#include <ostream>
#include <string_view>

namespace tarantino::crawler::model {

/// Why a book could not be ingested: exactly the four reasons of SPEC §5.
enum class FailureReason {
    NotFound,
    NetworkError,
    MissingMarkers,
    StorageError
};

/// The name the services print for the reason (SPEC §15): NOT_FOUND, NETWORK_ERROR, MISSING_MARKERS or STORAGE_ERROR.
constexpr std::string_view name(FailureReason reason) {
    switch (reason) {
        case FailureReason::NotFound:
            return "NOT_FOUND";
        case FailureReason::NetworkError:
            return "NETWORK_ERROR";
        case FailureReason::MissingMarkers:
            return "MISSING_MARKERS";
        case FailureReason::StorageError:
            return "STORAGE_ERROR";
    }
    return "UNKNOWN";
}

inline std::ostream& operator<<(std::ostream& out, FailureReason reason) {
    return out << name(reason);
}

}
