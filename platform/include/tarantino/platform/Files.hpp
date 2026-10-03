#pragma once

#include "tarantino/platform/Time.hpp"

#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

/// File operations as java.nio.file.Files offers them to the Java services, with UTF-8 contents and paths that work
/// on every operating system (Paths.hpp). Contents are written as they are, with no end-of-line conversion (SPEC §1).
/// Failures throw std::filesystem::filesystem_error, the C++ counterpart of UncheckedIOException.
namespace tarantino::platform::files {

using Path = std::filesystem::path;

/// The text of a file, which must be well-formed UTF-8, as Files.readString reads it (SPEC §1).
std::string readString(const Path& file);

/// The bytes of a file, as they are.
std::string readBytes(const Path& file);

/// The lines of a UTF-8 file, where only \n ends a line (SPEC §1).
std::vector<std::string> readLines(const Path& file);

/// Writes the text to the file, creating it or truncating it.
void writeString(const Path& file, std::string_view text);

/// Appends the text to the file, creating it if it does not exist.
void append(const Path& file, std::string_view text);

/// Writes a file in many pieces through a buffer, as a BufferedWriter does, without holding all of it in memory.
class Writer {
public:
    /// Creates the file, or truncates it.
    explicit Writer(const Path& file);

    Writer(const Writer&) = delete;
    Writer& operator=(const Writer&) = delete;

    /// Closes the file if close was not called, ignoring errors.
    ~Writer();

    void write(std::string_view text);

    /// Writes what is left in the buffer and closes the file, reporting any error.
    void close();

private:
    void drain();

    Path file_;
    std::FILE* handle_;
    std::string buffer_;
};

/// Moves a file over the target, which it replaces atomically on the same file system, as Files.move with ATOMIC_MOVE
/// and REPLACE_EXISTING does (SPEC §6, §14).
void move(const Path& source, const Path& target);

/// Copies a file to a target that must not exist yet, unless replaceExisting.
void copy(const Path& source, const Path& target, bool replaceExisting = false);

/// Whether the path exists, following symbolic links; false when it cannot be told, as Files.exists.
bool exists(const Path& path);

bool isDirectory(const Path& path);

bool isRegularFile(const Path& path);

bool isEmptyDirectory(const Path& directory);

/// Creates the directory and every missing parent, as Files.createDirectories.
void createDirectories(const Path& directory);

/// Deletes a file or an empty directory, which must exist, as Files.delete.
void remove(const Path& path);

/// Deletes a file or an empty directory if it exists, as Files.deleteIfExists, and tells whether it did.
bool removeIfExists(const Path& path);

/// Deletes a whole tree, if it exists.
void removeAll(const Path& path);

/// The entries of a directory, as Files.list: the directory joined with the name of each entry.
std::vector<Path> list(const Path& directory);

/// The names of the entries of a directory as UTF-8, in no particular order, without making a path of each entry as
/// list does: what a datalake needs to find its books among thousands of files. A path that is not a directory, or
/// does not exist, has none.
std::vector<std::string> names(const Path& directory);

/// What a walk tells of each path it meets.
struct WalkedPath {
    const Path& path;
    int depth;
    /// A directory, not reached through a symbolic link: the walk enters it.
    bool directory;
    /// A regular file, following symbolic links as Files.isRegularFile.
    bool regularFile;
    /// The entry of the operating system, to ask for more, such as its size, without reading the path again.
    const std::filesystem::directory_entry& entry;
};

/// Walks the tree depth first, each directory before its entries, as Files.walk does: the root, which must exist, has
/// depth 0, entries deeper than maxDepth are not visited, and the visitor stops the walk by returning false.
void walk(const Path& root, int maxDepth, const std::function<bool(const WalkedPath&)>& visitor);

/// Every path of the tree, the root first, as Files.walk lists them.
std::vector<Path> walk(const Path& root);

/// The first path of the tree, up to maxDepth, that satisfies the predicate, as Files.find(...).findFirst(): the walk
/// stops there (SPEC §6).
std::optional<Path> find(const Path& root, int maxDepth, const std::function<bool(const Path&)>& predicate);

/// The time the file was last modified, at the full precision of the file system (SPEC §6).
Instant lastModified(const Path& path);

/// The time the file was last modified, or none when it does not exist or cannot be read, as Files.exists and then
/// Files.getLastModifiedTime would tell, with one call to the operating system instead of two.
std::optional<Instant> lastModifiedIfExists(const Path& path);

/// Sets the time the file was last modified, leaving its access time unchanged.
void setLastModified(const Path& path, Instant instant);

std::uintmax_t size(const Path& file);

/// The block size of the file system that holds the path, as FileStore.getBlockSize: statvfs's f_frsize, or the
/// cluster size on Windows (SPEC §11).
std::uint64_t blockSize(const Path& path);

}
