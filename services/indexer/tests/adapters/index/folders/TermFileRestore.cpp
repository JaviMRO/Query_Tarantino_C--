#include "indexer/tests/adapters/index/folders/TermFileRestore.hpp"

#include "indexer/src/adapters/index/folders/TermFiles.hpp"
#include "tarantino/platform/Files.hpp"

#include <algorithm>
#include <atomic>
#include <exception>
#include <mutex>
#include <thread>
#include <vector>

namespace tarantino::indexer::adapters {

namespace files = platform::files;

void TermFileRestore::restore(const std::filesystem::path& snapshot, const std::filesystem::path& index,
                              const std::unordered_set<std::string>& terms) {
    const std::vector<std::string> pending(terms.begin(), terms.end());
    std::mutex foldersMutex;
    std::unordered_set<std::filesystem::path::string_type> folders;
    std::atomic<std::size_t> next{0};
    std::exception_ptr failure;
    const auto work = [&] {
        try {
            for (std::size_t term = next++; term < pending.size(); term = next++) {
                const std::filesystem::path stored = TermFiles::file(snapshot, pending[term]);
                const std::filesystem::path current = TermFiles::file(index, pending[term]);
                if (!files::exists(stored)) {
                    files::removeIfExists(current);
                    continue;
                }
                {
                    const std::lock_guard lock(foldersMutex);
                    if (folders.insert(current.parent_path().native()).second) {
                        files::createDirectories(current.parent_path());
                    }
                }
                files::copy(stored, current, true);
            }
        } catch (...) {
            const std::lock_guard lock(foldersMutex);
            failure = std::current_exception();
        }
    };
    std::vector<std::thread> workers;
    const unsigned threads = std::max(1u, std::thread::hardware_concurrency());
    for (unsigned worker = 1; worker < threads; ++worker) {
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
