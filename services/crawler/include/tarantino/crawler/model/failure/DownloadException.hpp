#pragma once

#include "tarantino/crawler/model/failure/FailureReason.hpp"

#include <stdexcept>
#include <string>

namespace tarantino::crawler::model {

/// A book that could not be downloaded or split, and why (SPEC §4, §5).
class DownloadException : public std::runtime_error {
public:
    DownloadException(FailureReason reason, const std::string& message) : std::runtime_error(message), reason_(reason) {
    }

    FailureReason reason() const {
        return reason_;
    }

private:
    FailureReason reason_;
};

}
