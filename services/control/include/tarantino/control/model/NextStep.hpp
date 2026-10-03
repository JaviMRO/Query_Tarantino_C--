#pragma once

#include <ostream>
#include <string>
#include <string_view>
#include <vector>

namespace tarantino::control::model {

/// What the control layer does next (SPEC §9), and to which books.
struct NextStep {
    enum class Action {
        Index,
        Download,
        Idle
    };

    Action action;
    std::vector<int> bookIds;

    static NextStep index(std::vector<int> bookIds) {
        return {Action::Index, std::move(bookIds)};
    }

    static NextStep download(int bookId) {
        return {Action::Download, {bookId}};
    }

    static NextStep idle() {
        return {Action::Idle, {}};
    }

    /// The book id, or <n> books (<first>…<last>) for a batch of more than one (SPEC §15).
    std::string books() const;

    bool operator==(const NextStep&) const = default;
};

/// INDEX, DOWNLOAD or IDLE.
std::string_view name(NextStep::Action action);

std::ostream& operator<<(std::ostream& out, const NextStep& step);

}
