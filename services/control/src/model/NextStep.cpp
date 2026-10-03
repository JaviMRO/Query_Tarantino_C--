#include "tarantino/control/model/NextStep.hpp"

namespace tarantino::control::model {

std::string NextStep::books() const {
    switch (bookIds.size()) {
        case 0:
            return "";
        case 1:
            return std::to_string(bookIds.front());
        default:
            // … is U+2026
            return std::to_string(bookIds.size()) + " books (" + std::to_string(bookIds.front()) + "\xE2\x80\xA6"
                   + std::to_string(bookIds.back()) + ")";
    }
}

std::string_view name(NextStep::Action action) {
    switch (action) {
        case NextStep::Action::Index:
            return "INDEX";
        case NextStep::Action::Download:
            return "DOWNLOAD";
        case NextStep::Action::Idle:
            return "IDLE";
    }
    return "UNKNOWN";
}

std::ostream& operator<<(std::ostream& out, const NextStep& step) {
    return out << name(step.action) << " " << step.books();
}

}
