#include "tarantino/indexer/model/book/HeaderParser.hpp"

#include "tarantino/platform/Text.hpp"

namespace tarantino::indexer::model {

Book HeaderParser::book(const BookText& text) const {
    return Book{text.bookId, field(text.header, "Title"), field(text.header, "Author"), field(text.header, "Language"),
                text.bodyPath};
}

std::optional<std::string> HeaderParser::field(std::string_view header, std::string_view name) {
    for (const std::string_view line : platform::lines(header)) {
        // A field needs a character after the colon: (.+) takes one even when it is a blank
        if (line.size() > name.size() + 1 && line.starts_with(name) && line[name.size()] == ':') {
            return std::string(platform::strip(line.substr(name.size() + 1)));
        }
    }
    return std::nullopt;
}

}
