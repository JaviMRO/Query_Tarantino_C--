#include "tarantino/platform/Paths.hpp"

#include <algorithm>

namespace tarantino::platform {

std::filesystem::path pathOf(std::string_view utf8) {
    return std::filesystem::path(std::u8string(utf8.begin(), utf8.end()));
}

std::string portable(const std::filesystem::path& path) {
    const std::u8string text = path.generic_u8string();
    return std::string(text.begin(), text.end());
}

std::string fileName(const std::filesystem::path& path) {
#ifdef _WIN32
    return portable(path.filename());
#else
    // The text after the last separator, which is what filename() is for a path without a root name
    const std::string& text = path.native();
    const std::size_t separator = text.rfind('/');
    return separator == std::string::npos ? text : text.substr(separator + 1);
#endif
}

#ifdef _WIN32
namespace {

bool isSeparator(wchar_t character) {
    return character == L'\\' || character == L'/';
}

/// Whether the path has a component that \\?\ would take literally and lexical normalization removes: . or .., or an
/// empty one between two separators (past the two that start a UNC name).
bool needsNormalizing(std::wstring_view text) {
    std::size_t start = 0;
    while (start <= text.size()) {
        std::size_t end = start;
        while (end < text.size() && !isSeparator(text[end])) {
            ++end;
        }
        const std::wstring_view component = text.substr(start, end - start);
        if (component == L"." || component == L".." || (component.empty() && start > 1 && end < text.size())) {
            return true;
        }
        start = end + 1;
    }
    return false;
}

}

std::wstring systemPathText(const std::filesystem::path& path) {
    const std::wstring& text = path.native();
    if (text.starts_with(L"\\\\?\\") || text.starts_with(L"\\\\.\\")) {
        return text;
    }
    // Normalized only when needed: it splits the path into its components, which costs more than the call it prepares
    // for the files of a folders index
    if (text.size() > 2 && isSeparator(text[0]) && isSeparator(text[1]) && !isSeparator(text[2])) {
        // \\server\share\…, which not every standard library parses as a root name
        std::wstring share = text.substr(2);
        if (needsNormalizing(share)) {
            share = std::filesystem::path(share).lexically_normal().native();
        }
        std::replace(share.begin(), share.end(), L'/', L'\\');
        return L"\\\\?\\UNC\\" + share;
    }
    // Not std::filesystem::absolute: GetFullPathNameW turns a file named con.txt into the console device \\.\con
    std::wstring absolute = path.is_absolute() ? text : (std::filesystem::current_path() / path).native();
    if (needsNormalizing(absolute)) {
        absolute = std::filesystem::path(absolute).lexically_normal().native();
    }
    std::replace(absolute.begin(), absolute.end(), L'/', L'\\');
    return L"\\\\?\\" + absolute;
}

std::filesystem::path systemPath(const std::filesystem::path& path) {
    return std::filesystem::path(systemPathText(path));
}
#else
std::filesystem::path systemPath(const std::filesystem::path& path) {
    return path;
}
#endif

}
