#include "adapters/index/folders/TermFiles.hpp"

#include "tarantino/platform/Sha256.hpp"
#include "tarantino/platform/Utf8.hpp"

#include <string>

namespace tarantino::indexer::adapters {

namespace {

constexpr const char* SUFFIX = ".txt";
constexpr std::size_t MAX_NAME_LENGTH = 200;
constexpr const char* HASH_PREFIX = "#";

std::string encoded(std::string_view text) {
    static constexpr char HEX[] = "0123456789ABCDEF";
    std::string name;
    name.reserve(text.size());
    for (const char character : text) {
        const auto byte = static_cast<unsigned char>(character);
        if (byte >= 'a' && byte <= 'z') {
            name.push_back(character);
        } else {
            name.push_back('%');
            name.push_back(HEX[byte >> 4]);
            name.push_back(HEX[byte & 0x0F]);
        }
    }
    return name;
}

std::string name(std::string_view term) {
    std::string encodedTerm = encoded(term);
    if (encodedTerm.size() <= MAX_NAME_LENGTH) {
        return encodedTerm;
    }
    const auto digest = platform::sha256(term);
    return HASH_PREFIX + platform::hex(digest.data(), digest.size());
}

std::string_view firstCharacter(std::string_view term) {
    return term.substr(0, platform::utf8::sequenceLength(static_cast<unsigned char>(term.front())));
}

}

std::filesystem::path TermFiles::file(const std::filesystem::path& root, std::string_view term) {
    return root / encoded(firstCharacter(term)) / (name(term) + SUFFIX);
}

}
