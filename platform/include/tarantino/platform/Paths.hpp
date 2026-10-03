#pragma once

#include <filesystem>
#include <string>
#include <string_view>

/// Paths built from and written as UTF-8, on every operating system.
namespace tarantino::platform {

/// The path named by UTF-8 text. A plain std::string would be read in the ANSI code page on Windows.
std::filesystem::path pathOf(std::string_view utf8);

/// The path as UTF-8 text with / as separator, the way SPEC §1 stores and prints paths on every operating system.
std::string portable(const std::filesystem::path& path);

/// The name of the last component of the path as UTF-8, portable(path.filename()) without building a path for it.
std::string fileName(const std::filesystem::path& path);

/// The path as the operating system must receive it in file system calls. On Windows it is made absolute and given the
/// \\?\ prefix, so that neither the 260-character limit nor the reserved device names (con, nul, aux, prn), which are
/// also terms of the folders index (SPEC §12), apply. Elsewhere it is the path itself.
std::filesystem::path systemPath(const std::filesystem::path& path);

#ifdef _WIN32
/// The text of systemPath, for the calls of the Windows API, without the cost of making a path of it.
std::wstring systemPathText(const std::filesystem::path& path);
#endif

}
