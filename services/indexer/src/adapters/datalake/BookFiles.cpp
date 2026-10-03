#include "tarantino/indexer/adapters/datalake/BookFiles.hpp"

#include "tarantino/platform/Files.hpp"

namespace tarantino::indexer::adapters {

std::optional<model::BookText> BookFiles::bookText(int bookId, const std::filesystem::path& header,
                                                   const std::filesystem::path& body) {
    if (!platform::files::exists(body)) {
        return std::nullopt;
    }
    return model::BookText{bookId, platform::files::readString(header), platform::files::readString(body), body};
}

}
