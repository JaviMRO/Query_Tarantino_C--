#include "crawler/tests/benchmarking/support/files/Directories.hpp"

#include "tarantino/platform/Files.hpp"

#include <algorithm>
#include <atomic>
#include <exception>
#include <limits>
#include <mutex>
#include <thread>
#include <vector>

namespace tarantino::crawler::benchmarking {

namespace files = platform::files;
using std::filesystem::path;

namespace {

constexpr int WHOLE_TREE = std::numeric_limits<int>::max();

/// The directories of a tree, each before its contents, and its other entries, listed in one walk.
struct Tree {
    std::vector<path> directories;
    std::vector<path> files;

    static Tree of(const path& root) {
        Tree tree;
        if (!files::exists(root)) {
            return tree;
        }
        files::walk(root, WHOLE_TREE, [&](const files::WalkedPath& walked) {
            (walked.directory ? tree.directories : tree.files).push_back(walked.path);
            return true;
        });
        return tree;
    }
};

/// Runs the action on every path with as many threads as the machine has, as Java's parallel streams do.
void inParallel(const std::vector<path>& paths, const std::function<void(const path&)>& action) {
    const std::size_t threads = std::max<std::size_t>(1, std::min<std::size_t>(std::thread::hardware_concurrency(), paths.size() / 64 + 1));
    std::atomic<std::size_t> next{0};
    std::exception_ptr failure;
    std::mutex failureMutex;
    const auto work = [&] {
        try {
            for (std::size_t index = next++; index < paths.size(); index = next++) {
                action(paths[index]);
            }
        } catch (...) {
            const std::lock_guard lock(failureMutex);
            failure = std::current_exception();
        }
    };
    std::vector<std::thread> workers;
    for (std::size_t worker = 1; worker < threads; ++worker) {
        workers.emplace_back(work);
    }
    work();
    for (std::thread& worker : workers) {
        worker.join();
    }
    if (failure) {
        std::rethrow_exception(failure);
    }
}

}

void Directories::remove(const path& root) {
    const Tree tree = Tree::of(root);
    inParallel(tree.files, [](const path& file) { files::remove(file); });
    for (auto directory = tree.directories.rbegin(); directory != tree.directories.rend(); ++directory) {
        files::remove(*directory);
    }
}

void Directories::copy(const path& source, const path& target) {
    const Tree tree = Tree::of(source);
    for (const path& directory : tree.directories) {
        files::createDirectories(target / directory.lexically_relative(source));
    }
    inParallel(tree.files, [&](const path& file) { files::copy(file, target / file.lexically_relative(source)); });
}

std::uint64_t Directories::fileCount(const path& root, const std::function<bool(const path&)>& filter) {
    std::uint64_t count = 0;
    if (!files::exists(root)) {
        return count;
    }
    files::walk(root, WHOLE_TREE, [&](const files::WalkedPath& walked) {
        if (walked.regularFile && (!filter || filter(walked.path))) {
            ++count;
        }
        return true;
    });
    return count;
}

Footprint Directories::footprint(const path& root) {
    Footprint footprint;
    if (!files::exists(root)) {
        return footprint;
    }
    const std::uint64_t block = files::blockSize(root);
    files::walk(root, WHOLE_TREE, [&](const files::WalkedPath& walked) {
        if (walked.directory && walked.depth > 0) {
            ++footprint.directories;
        } else if (walked.regularFile) {
            std::error_code error;
            const std::uint64_t size = walked.entry.file_size(error);
            ++footprint.files;
            footprint.bytes += size;
            footprint.allocatedBytes += (size + block - 1) / block * block;
        }
        return true;
    });
    return footprint;
}

}
