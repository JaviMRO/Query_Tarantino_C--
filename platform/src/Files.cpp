#include "tarantino/platform/Files.hpp"

#include "tarantino/platform/Paths.hpp"
#include "tarantino/platform/Text.hpp"
#include "tarantino/platform/Utf8.hpp"

#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <limits>
#include <memory>
#include <system_error>
#include <type_traits>

#ifdef _WIN32
#include <windows.h>
#else
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <unistd.h>
#endif

namespace tarantino::platform::files {

namespace {

[[noreturn]] void fail(const char* what, const Path& path, std::error_code error) {
    throw std::filesystem::filesystem_error(what, path, error);
}

[[noreturn]] void fail(const char* what, const Path& source, const Path& target, std::error_code error) {
    throw std::filesystem::filesystem_error(what, source, target, error);
}

std::error_code lastError() {
    return {errno, std::generic_category()};
}

#ifdef _WIN32
std::error_code lastWindowsError() {
    return {static_cast<int>(GetLastError()), std::system_category()};
}

// FILETIME counts 100-nanosecond intervals since 1 January 1601
constexpr std::int64_t FILETIME_OF_EPOCH = 116'444'736'000'000'000LL;
#endif

struct FileCloser {
    void operator()(std::FILE* file) const {
        std::fclose(file);
    }
};

using File = std::unique_ptr<std::FILE, FileCloser>;

File open(const Path& path, const char* mode) {
#ifdef _WIN32
    std::wstring wideMode(mode, mode + std::char_traits<char>::length(mode));
    return File(_wfopen(systemPathText(path).c_str(), wideMode.c_str()));
#else
    return File(std::fopen(path.c_str(), mode));
#endif
}

// Whole files are read and written with the calls of the operating system, as Java's NIO does, without the buffer
// of a FILE*: a folders index has hundreds of thousands of small files.
constexpr std::size_t LARGEST_CALL = std::size_t{1} << 30;

#ifdef _WIN32
struct HandleCloser {
    void operator()(HANDLE handle) const {
        CloseHandle(handle);
    }
};

using Native = std::unique_ptr<std::remove_pointer_t<HANDLE>, HandleCloser>;

Native openNative(const Path& path, DWORD access, DWORD disposition) {
    HANDLE handle = CreateFileW(systemPathText(path).c_str(), access, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                nullptr, disposition, FILE_ATTRIBUTE_NORMAL, nullptr);
    return Native(handle == INVALID_HANDLE_VALUE ? nullptr : handle);
}

DWORD attributes(const Path& path) {
    return GetFileAttributesW(systemPathText(path).c_str());
}
#else
struct Descriptor {
    int value;

    ~Descriptor() {
        if (value >= 0) {
            ::close(value);
        }
    }
};
#endif

void write(const Path& file, std::string_view text, bool append) {
    std::size_t done = 0;
#ifdef _WIN32
    const Native handle = openNative(file, append ? FILE_APPEND_DATA : GENERIC_WRITE, append ? OPEN_ALWAYS : CREATE_ALWAYS);
    if (!handle) {
        fail("Could not open the file to write", file, lastWindowsError());
    }
    while (done < text.size()) {
        DWORD written = 0;
        const auto piece = static_cast<DWORD>(std::min(text.size() - done, LARGEST_CALL));
        if (!WriteFile(handle.get(), text.data() + done, piece, &written, nullptr)) {
            fail("Could not write the file", file, lastWindowsError());
        }
        done += written;
    }
#else
    Descriptor descriptor{::open(file.c_str(), O_WRONLY | O_CREAT | O_CLOEXEC | (append ? O_APPEND : O_TRUNC), 0666)};
    if (descriptor.value < 0) {
        fail("Could not open the file to write", file, lastError());
    }
    while (done < text.size()) {
        const ssize_t written = ::write(descriptor.value, text.data() + done, std::min(text.size() - done, LARGEST_CALL));
        if (written < 0 && errno == EINTR) {
            continue;
        }
        if (written <= 0) {
            fail("Could not write the file", file, lastError());
        }
        done += static_cast<std::size_t>(written);
    }
    const int closed = ::close(descriptor.value);
    descriptor.value = -1;
    if (closed != 0) {
        fail("Could not write the file", file, lastError());
    }
#endif
}

std::int64_t floorDivide(std::int64_t dividend, std::int64_t divisor) {
    std::int64_t quotient = dividend / divisor;
    if (dividend % divisor != 0 && ((dividend < 0) != (divisor < 0))) {
        --quotient;
    }
    return quotient;
}

bool walkDirectory(const Path& directory, const Path& systemDirectory, int depth, int maxDepth,
                   const std::function<bool(const WalkedPath&)>& visitor) {
    std::error_code error;
    std::filesystem::directory_iterator entries(systemDirectory, error);
    const std::filesystem::directory_iterator end;
    for (; !error && entries != end; entries.increment(error)) {
        const std::filesystem::directory_entry& entry = *entries;
#ifdef _WIN32
        // The entry is under the system path of the directory; the walk shows it under the path it was given
        const Path path = directory / entry.path().filename();
#else
        const Path& path = entry.path();
#endif
        std::error_code typeError;
        const bool link = entry.is_symlink(typeError);
        const bool directoryEntry = !link && entry.is_directory(typeError);
        const bool regularFile = entry.is_regular_file(typeError);
        if (!visitor(WalkedPath{path, depth + 1, directoryEntry, regularFile, entry})) {
            return false;
        }
        if (directoryEntry && depth + 1 < maxDepth && !walkDirectory(path, entry.path(), depth + 1, maxDepth, visitor)) {
            return false;
        }
    }
    if (error) {
        fail("Could not list the directory", directory, error);
    }
    return true;
}

}

std::string readBytes(const Path& file) {
    // Room for the size the file has and one more byte, so that the read that finds its end needs no other buffer
    std::string bytes;
    std::size_t done = 0;
#ifdef _WIN32
    const Native handle = openNative(file, GENERIC_READ, OPEN_EXISTING);
    if (!handle) {
        fail("Could not open the file to read", file, lastWindowsError());
    }
    LARGE_INTEGER size{};
    GetFileSizeEx(handle.get(), &size);
    bytes.resize(static_cast<std::size_t>(size.QuadPart) + 1);
    while (true) {
        if (done == bytes.size()) {
            bytes.resize(bytes.size() * 2);
        }
        DWORD read = 0;
        const auto piece = static_cast<DWORD>(std::min(bytes.size() - done, LARGEST_CALL));
        if (!ReadFile(handle.get(), bytes.data() + done, piece, &read, nullptr)) {
            fail("Could not read the file", file, lastWindowsError());
        }
        if (read == 0) {
            break;
        }
        done += read;
    }
#else
    const Descriptor descriptor{::open(file.c_str(), O_RDONLY | O_CLOEXEC)};
    if (descriptor.value < 0) {
        fail("Could not open the file to read", file, lastError());
    }
    struct stat status {};
    ::fstat(descriptor.value, &status);
    bytes.resize(static_cast<std::size_t>(status.st_size) + 1);
    while (true) {
        if (done == bytes.size()) {
            bytes.resize(bytes.size() * 2);
        }
        const ssize_t read = ::read(descriptor.value, bytes.data() + done, std::min(bytes.size() - done, LARGEST_CALL));
        if (read < 0 && errno == EINTR) {
            continue;
        }
        if (read < 0) {
            fail("Could not read the file", file, lastError());
        }
        if (read == 0) {
            break;
        }
        done += static_cast<std::size_t>(read);
    }
#endif
    bytes.resize(done);
    return bytes;
}

std::string readString(const Path& file) {
    std::string text = readBytes(file);
    if (!utf8::isValid(text)) {
        fail("Input is not well-formed UTF-8", file, std::make_error_code(std::errc::illegal_byte_sequence));
    }
    return text;
}

std::vector<std::string> readLines(const Path& file) {
    const std::string text = readString(file);
    std::vector<std::string> result;
    for (const std::string_view line : lines(text)) {
        result.emplace_back(line);
    }
    return result;
}

void writeString(const Path& file, std::string_view text) {
    write(file, text, false);
}

void append(const Path& file, std::string_view text) {
    write(file, text, true);
}

namespace {
constexpr std::size_t WRITER_BUFFER = 1 << 16;
}

Writer::Writer(const Path& file) : file_(file), handle_(open(file, "wb").release()) {
    if (handle_ == nullptr) {
        fail("Could not open the file to write", file, lastError());
    }
    buffer_.reserve(WRITER_BUFFER);
}

Writer::~Writer() {
    if (handle_ != nullptr) {
        std::fclose(handle_);
    }
}

void Writer::write(std::string_view text) {
    buffer_.append(text);
    if (buffer_.size() >= WRITER_BUFFER) {
        drain();
    }
}

void Writer::close() {
    drain();
    std::FILE* handle = handle_;
    handle_ = nullptr;
    if (std::fclose(handle) != 0) {
        fail("Could not write the file", file_, lastError());
    }
}

void Writer::drain() {
    if (!buffer_.empty() && std::fwrite(buffer_.data(), 1, buffer_.size(), handle_) != buffer_.size()) {
        fail("Could not write the file", file_, lastError());
    }
    buffer_.clear();
}

void move(const Path& source, const Path& target) {
#ifdef _WIN32
    if (!MoveFileExW(systemPathText(source).c_str(), systemPathText(target).c_str(), MOVEFILE_REPLACE_EXISTING)) {
        fail("Could not move the file", source, target, lastWindowsError());
    }
#else
    std::error_code error;
    std::filesystem::rename(source, target, error);
    if (error) {
        fail("Could not move the file", source, target, error);
    }
#endif
}

void copy(const Path& source, const Path& target, bool replaceExisting) {
    std::error_code error;
    const auto options = replaceExisting ? std::filesystem::copy_options::overwrite_existing : std::filesystem::copy_options::none;
    std::filesystem::copy_file(systemPath(source), systemPath(target), options, error);
    if (error) {
        fail("Could not copy the file", source, target, error);
    }
}

// The attributes alone, without opening the file, as Java reads them
bool exists(const Path& path) {
#ifdef _WIN32
    return attributes(path) != INVALID_FILE_ATTRIBUTES;
#else
    struct stat status {};
    return ::stat(path.c_str(), &status) == 0;
#endif
}

bool isDirectory(const Path& path) {
#ifdef _WIN32
    const DWORD found = attributes(path);
    return found != INVALID_FILE_ATTRIBUTES && (found & FILE_ATTRIBUTE_DIRECTORY) != 0;
#else
    struct stat status {};
    return ::stat(path.c_str(), &status) == 0 && S_ISDIR(status.st_mode);
#endif
}

bool isRegularFile(const Path& path) {
#ifdef _WIN32
    const DWORD found = attributes(path);
    return found != INVALID_FILE_ATTRIBUTES && (found & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_DEVICE)) == 0;
#else
    struct stat status {};
    return ::stat(path.c_str(), &status) == 0 && S_ISREG(status.st_mode);
#endif
}

bool isEmptyDirectory(const Path& directory) {
    std::error_code error;
    const bool empty = std::filesystem::is_empty(systemPath(directory), error);
    if (error) {
        fail("Could not list the directory", directory, error);
    }
    return empty;
}

void createDirectories(const Path& directory) {
    std::error_code error;
    std::filesystem::create_directories(systemPath(directory), error);
    if (error) {
        fail("Could not create the directory", directory, error);
    }
}

void remove(const Path& path) {
    std::error_code error;
    if (!std::filesystem::remove(systemPath(path), error)) {
        fail("Could not delete", path, error ? error : std::make_error_code(std::errc::no_such_file_or_directory));
    }
}

bool removeIfExists(const Path& path) {
    std::error_code error;
    const bool removed = std::filesystem::remove(systemPath(path), error);
    if (error) {
        fail("Could not delete", path, error);
    }
    return removed;
}

void removeAll(const Path& path) {
    std::error_code error;
    std::filesystem::remove_all(systemPath(path), error);
    if (error) {
        fail("Could not delete the tree", path, error);
    }
}

std::vector<Path> list(const Path& directory) {
    std::vector<Path> children;
    std::error_code error;
    std::filesystem::directory_iterator entries(systemPath(directory), error);
    const std::filesystem::directory_iterator end;
    for (; !error && entries != end; entries.increment(error)) {
#ifdef _WIN32
        children.push_back(directory / entries->path().filename());
#else
        // The directory joined with the name, as the iterator already gives it
        children.push_back(entries->path());
#endif
    }
    if (error) {
        fail("Could not list the directory", directory, error);
    }
    return children;
}

std::vector<std::string> names(const Path& directory) {
    std::vector<std::string> found;
#ifdef _WIN32
    std::wstring pattern = systemPathText(directory);
    if (!pattern.ends_with(L'\\')) {
        pattern += L'\\';
    }
    pattern += L'*';
    WIN32_FIND_DATAW entry;
    HANDLE search = FindFirstFileExW(pattern.c_str(), FindExInfoBasic, &entry, FindExSearchNameMatch, nullptr, FIND_FIRST_EX_LARGE_FETCH);
    if (search == INVALID_HANDLE_VALUE) {
        const DWORD error = GetLastError();
        if (error == ERROR_FILE_NOT_FOUND || error == ERROR_PATH_NOT_FOUND || error == ERROR_DIRECTORY) {
            return found;
        }
        fail("Could not list the directory", directory, std::error_code(static_cast<int>(error), std::system_category()));
    }
    do {
        const std::wstring_view name(entry.cFileName);
        if (name != L"." && name != L"..") {
            found.push_back(portable(std::filesystem::path(name)));
        }
    } while (FindNextFileW(search, &entry));
    const DWORD error = GetLastError();
    FindClose(search);
    if (error != ERROR_NO_MORE_FILES) {
        fail("Could not list the directory", directory, std::error_code(static_cast<int>(error), std::system_category()));
    }
#else
    DIR* stream = ::opendir(directory.c_str());
    if (stream == nullptr) {
        if (errno == ENOENT || errno == ENOTDIR) {
            return found;
        }
        fail("Could not list the directory", directory, lastError());
    }
    const std::unique_ptr<DIR, int (*)(DIR*)> closer(stream, ::closedir);
    while (true) {
        errno = 0;
        const dirent* entry = ::readdir(stream);
        if (entry == nullptr) {
            break;
        }
        const std::string_view name(entry->d_name);
        if (name != "." && name != "..") {
            found.emplace_back(name);
        }
    }
    if (errno != 0) {
        fail("Could not list the directory", directory, lastError());
    }
#endif
    return found;
}

void walk(const Path& root, int maxDepth, const std::function<bool(const WalkedPath&)>& visitor) {
    const Path systemRoot = systemPath(root);
    std::error_code error;
    const std::filesystem::directory_entry rootEntry(systemRoot, error);
    if (error || !rootEntry.exists()) {
        fail("Could not walk the tree", root, error ? error : std::make_error_code(std::errc::no_such_file_or_directory));
    }
    const bool directory = rootEntry.is_directory(error);
    if (!visitor(WalkedPath{root, 0, directory, rootEntry.is_regular_file(error), rootEntry})) {
        return;
    }
    if (directory && maxDepth > 0) {
        walkDirectory(root, systemRoot, 0, maxDepth, visitor);
    }
}

std::vector<Path> walk(const Path& root) {
    std::vector<Path> paths;
    walk(root, std::numeric_limits<int>::max(), [&](const WalkedPath& walked) {
        paths.push_back(walked.path);
        return true;
    });
    return paths;
}

std::optional<Path> find(const Path& root, int maxDepth, const std::function<bool(const Path&)>& predicate) {
    std::optional<Path> found;
    walk(root, maxDepth, [&](const WalkedPath& walked) {
        if (predicate(walked.path)) {
            found = walked.path;
            return false;
        }
        return true;
    });
    return found;
}

Instant lastModified(const Path& path) {
#ifdef _WIN32
    WIN32_FILE_ATTRIBUTE_DATA attributes;
    if (!GetFileAttributesExW(systemPathText(path).c_str(), GetFileExInfoStandard, &attributes)) {
        fail("Could not read the modification time", path, lastWindowsError());
    }
    ULARGE_INTEGER ticks;
    ticks.LowPart = attributes.ftLastWriteTime.dwLowDateTime;
    ticks.HighPart = attributes.ftLastWriteTime.dwHighDateTime;
    return Instant(std::chrono::nanoseconds((static_cast<std::int64_t>(ticks.QuadPart) - FILETIME_OF_EPOCH) * 100));
#else
    struct stat status {};
    if (::stat(path.c_str(), &status) != 0) {
        fail("Could not read the modification time", path, lastError());
    }
#ifdef __APPLE__
    const struct timespec& time = status.st_mtimespec;
#else
    const struct timespec& time = status.st_mtim;
#endif
    return Instant(std::chrono::seconds(time.tv_sec) + std::chrono::nanoseconds(time.tv_nsec));
#endif
}

std::optional<Instant> lastModifiedIfExists(const Path& path) {
#ifdef _WIN32
    WIN32_FILE_ATTRIBUTE_DATA attributes;
    if (!GetFileAttributesExW(systemPathText(path).c_str(), GetFileExInfoStandard, &attributes)) {
        return std::nullopt;
    }
    ULARGE_INTEGER ticks;
    ticks.LowPart = attributes.ftLastWriteTime.dwLowDateTime;
    ticks.HighPart = attributes.ftLastWriteTime.dwHighDateTime;
    return Instant(std::chrono::nanoseconds((static_cast<std::int64_t>(ticks.QuadPart) - FILETIME_OF_EPOCH) * 100));
#else
    struct stat status {};
    if (::stat(path.c_str(), &status) != 0) {
        return std::nullopt;
    }
#ifdef __APPLE__
    const struct timespec& time = status.st_mtimespec;
#else
    const struct timespec& time = status.st_mtim;
#endif
    return Instant(std::chrono::seconds(time.tv_sec) + std::chrono::nanoseconds(time.tv_nsec));
#endif
}

void setLastModified(const Path& path, Instant instant) {
    const std::int64_t nanos = instant.time_since_epoch().count();
#ifdef _WIN32
    HANDLE handle = CreateFileW(systemPathText(path).c_str(), FILE_WRITE_ATTRIBUTES,
                                FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING,
                                FILE_FLAG_BACKUP_SEMANTICS, nullptr);
    if (handle == INVALID_HANDLE_VALUE) {
        fail("Could not set the modification time", path, lastWindowsError());
    }
    const auto ticks = static_cast<std::uint64_t>(floorDivide(nanos, 100) + FILETIME_OF_EPOCH);
    FILETIME time;
    time.dwLowDateTime = static_cast<DWORD>(ticks & 0xFFFFFFFFULL);
    time.dwHighDateTime = static_cast<DWORD>(ticks >> 32);
    const BOOL set = SetFileTime(handle, nullptr, nullptr, &time);
    const std::error_code error = lastWindowsError();
    CloseHandle(handle);
    if (!set) {
        fail("Could not set the modification time", path, error);
    }
#else
    constexpr std::int64_t nanosPerSecond = 1'000'000'000;
    const std::int64_t seconds = floorDivide(nanos, nanosPerSecond);
    struct timespec times[2] {};
    times[0].tv_nsec = UTIME_OMIT;
    times[1].tv_sec = static_cast<time_t>(seconds);
    times[1].tv_nsec = static_cast<long>(nanos - seconds * nanosPerSecond);
    if (::utimensat(AT_FDCWD, path.c_str(), times, 0) != 0) {
        fail("Could not set the modification time", path, lastError());
    }
#endif
}

std::uintmax_t size(const Path& file) {
    std::error_code error;
    const std::uintmax_t bytes = std::filesystem::file_size(systemPath(file), error);
    if (error) {
        fail("Could not read the size", file, error);
    }
    return bytes;
}

std::uint64_t blockSize(const Path& path) {
#ifdef _WIN32
    wchar_t volume[MAX_PATH + 1];
    if (!GetVolumePathNameW(systemPathText(path).c_str(), volume, MAX_PATH + 1)) {
        fail("Could not find the volume", path, lastWindowsError());
    }
    DWORD sectorsPerCluster = 0;
    DWORD bytesPerSector = 0;
    DWORD freeClusters = 0;
    DWORD clusters = 0;
    if (!GetDiskFreeSpaceW(volume, &sectorsPerCluster, &bytesPerSector, &freeClusters, &clusters)) {
        fail("Could not read the cluster size", path, lastWindowsError());
    }
    return static_cast<std::uint64_t>(sectorsPerCluster) * bytesPerSector;
#else
    struct statvfs stats {};
    if (::statvfs(path.c_str(), &stats) != 0) {
        fail("Could not read the file system", path, lastError());
    }
    return static_cast<std::uint64_t>(stats.f_frsize);
#endif
}

}
