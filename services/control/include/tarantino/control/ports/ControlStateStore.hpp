#pragma once

#include <vector>

namespace tarantino::control::ports {

/// The state of the control layer (SPEC §9): which books were downloaded and which indexed, appended book by book.
class ControlStateStore {
public:
    virtual ~ControlStateStore() = default;

    /// The distinct ids, in the order they were marked.
    virtual std::vector<int> downloaded() = 0;

    virtual std::vector<int> indexed() = 0;

    virtual void markDownloaded(int bookId) = 0;

    virtual void markIndexed(int bookId) = 0;
};

}
