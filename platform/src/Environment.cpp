#include "tarantino/platform/Environment.hpp"

#include <cstdio>
#include <cstdlib>
#include <iostream>

#ifdef _WIN32
// clang-format off
#include <windows.h>
#include <shellapi.h>
// clang-format on
#endif

namespace tarantino::platform {

namespace {

#ifdef _WIN32
std::wstring wide(const std::string& text) {
    if (text.empty()) {
        return {};
    }
    const int length = MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), nullptr, 0);
    std::wstring result(static_cast<std::size_t>(length), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), result.data(), length);
    return result;
}

std::string narrow(const wchar_t* text) {
    const int length = WideCharToMultiByte(CP_UTF8, 0, text, -1, nullptr, 0, nullptr, nullptr);
    if (length <= 1) {
        return {};
    }
    std::string result(static_cast<std::size_t>(length), '\0');
    WideCharToMultiByte(CP_UTF8, 0, text, -1, result.data(), length, nullptr, nullptr);
    result.pop_back();
    return result;
}
#endif

}

std::optional<std::string> environmentVariable(const std::string& name) {
#ifdef _WIN32
    const wchar_t* value = _wgetenv(wide(name).c_str());
    if (value == nullptr) {
        return std::nullopt;
    }
    return narrow(value);
#else
    const char* value = std::getenv(name.c_str());
    if (value == nullptr) {
        return std::nullopt;
    }
    return std::string(value);
#endif
}

std::string environmentVariable(const std::string& name, const std::string& defaultValue) {
    return environmentVariable(name).value_or(defaultValue);
}

std::vector<std::string> commandLineArguments(int argc, char** argv) {
    std::vector<std::string> arguments;
#ifdef _WIN32
    int count = 0;
    LPWSTR* wideArguments = CommandLineToArgvW(GetCommandLineW(), &count);
    if (wideArguments != nullptr) {
        for (int index = 1; index < count; ++index) {
            arguments.push_back(narrow(wideArguments[index]));
        }
        LocalFree(wideArguments);
        return arguments;
    }
#endif
    for (int index = 1; index < argc; ++index) {
        arguments.emplace_back(argv[index]);
    }
    return arguments;
}

void useUtf8Console() {
#ifdef _WIN32
    // The code page belongs to the console, which outlives the process, so the one it had is put back at exit, once
    // everything written has reached the console
    static const struct Utf8Console {
        UINT previous = GetConsoleOutputCP();

        Utf8Console() {
            SetConsoleOutputCP(CP_UTF8);
        }

        ~Utf8Console() {
            std::cout.flush();
            std::cerr.flush();
            std::fflush(nullptr);
            if (previous != 0) {
                SetConsoleOutputCP(previous);
            }
        }
    } console;
#endif
}

}
