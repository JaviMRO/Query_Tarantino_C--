#include "indexer/tests/benchmarking/support/PrebuiltIndexes.hpp"

#include "crawler/tests/benchmarking/support/environment/BenchmarkPaths.hpp"
#include "crawler/tests/benchmarking/support/files/Directories.hpp"
#include "tarantino/platform/Files.hpp"
#include "tarantino/platform/Paths.hpp"

namespace tarantino::indexer::benchmarking {

namespace files = platform::files;
using crawler::benchmarking::BenchmarkPaths;
using crawler::benchmarking::Directories;

namespace {

constexpr const char* PREFIX = "prebuilt-";
constexpr const char* BUILT_MARKER = ".built";
constexpr const char* WORKING_MARKER = ".working";

BenchmarkStore builtStore(const std::string& index, int books) {
    return BenchmarkStore::forIndex(index, std::string(PREFIX) + "index-" + index + "-" + std::to_string(books));
}

BenchmarkStore workingStore(const std::string& index, int books) {
    return BenchmarkStore::forIndex(index, std::string(PREFIX) + "working-" + index + "-" + std::to_string(books));
}

std::filesystem::path marker(const std::string& index, int books, const std::string& kind) {
    return PrebuiltIndexes::file(index + "-" + std::to_string(books) + kind);
}

void create(const std::filesystem::path& marker) {
    files::createDirectories(marker.parent_path());
    files::writeString(marker, "");
}

/// The store a marker stands for, cleared before the marker goes: prebuilt-<index>-<books>.built or .working.
void deleteStoreOf(const std::filesystem::path& entry) {
    const std::string name = platform::portable(entry.filename());
    for (const std::string kind : {BUILT_MARKER, WORKING_MARKER}) {
        if (!name.ends_with(kind)) {
            continue;
        }
        const std::string indexAndBooks = name.substr(std::string(PREFIX).size(), name.size() - std::string(PREFIX).size() - kind.size());
        const std::size_t dash = indexAndBooks.find('-');
        const std::string index = indexAndBooks.substr(0, dash);
        const int books = std::stoi(indexAndBooks.substr(dash + 1));
        (kind == BUILT_MARKER ? builtStore(index, books) : workingStore(index, books)).clear();
    }
}

std::vector<std::filesystem::path> prebuiltEntries() {
    std::vector<std::filesystem::path> prebuilt;
    const std::filesystem::path root = BenchmarkPaths::scratchRoot();
    if (!files::isDirectory(root)) {
        return prebuilt;
    }
    for (const std::filesystem::path& entry : files::list(root)) {
        if (platform::portable(entry.filename()).starts_with(PREFIX)) {
            prebuilt.push_back(entry);
        }
    }
    return prebuilt;
}

}

BenchmarkStore PrebuiltIndexes::of(const std::string& index, int books, IndexFixture& fixture) {
    BenchmarkStore store = builtStore(index, books);
    const std::filesystem::path built = marker(index, books, BUILT_MARKER);
    if (!files::exists(built)) {
        store.clear();
        fixture.index(*store.invertedIndex(), fixture.dataset().ids(books));
        create(built);
    }
    return store;
}

BenchmarkStore PrebuiltIndexes::workingCopyOf(const BenchmarkStore& prebuilt, const std::string& index, int books) {
    BenchmarkStore store = workingStore(index, books);
    const std::filesystem::path working = marker(index, books, WORKING_MARKER);
    if (!files::exists(working)) {
        prebuilt.copyTo(store);
        create(working);
    }
    return store;
}

std::filesystem::path PrebuiltIndexes::file(const std::string& name) {
    return BenchmarkPaths::scratch(PREFIX + name);
}

void PrebuiltIndexes::deleteAll() {
    for (const std::filesystem::path& entry : prebuiltEntries()) {
        deleteStoreOf(entry);
    }
    for (const std::filesystem::path& entry : prebuiltEntries()) {
        Directories::remove(entry);
    }
}

}
