#include "crawler/tests/benchmarking/support/environment/Heap.hpp"

#if defined(_WIN32)
#include <windows.h>
#include <vector>
#elif defined(__APPLE__)
#include <malloc/malloc.h>
#elif defined(__GLIBC__)
#include <malloc.h>
#endif

namespace tarantino::crawler::benchmarking {

// Constant-initialized, so it counts from the first allocation of the process
std::atomic<std::uint64_t> Heap::allocated_{0};

std::uint64_t Heap::inUse() {
#if defined(_WIN32)
    // Every allocation of the C runtime ends in a heap of the process; their busy blocks are what is in use
    const DWORD count = GetProcessHeaps(0, nullptr);
    std::vector<HANDLE> heaps(count);
    GetProcessHeaps(count, heaps.data());
    std::uint64_t used = 0;
    for (HANDLE heap : heaps) {
        if (!HeapLock(heap)) {
            continue;
        }
        PROCESS_HEAP_ENTRY entry{};
        while (HeapWalk(heap, &entry)) {
            if ((entry.wFlags & PROCESS_HEAP_ENTRY_BUSY) != 0) {
                used += entry.cbData;
            }
        }
        HeapUnlock(heap);
    }
    return used;
#elif defined(__APPLE__)
    malloc_statistics_t statistics{};
    malloc_zone_statistics(nullptr, &statistics);
    return statistics.size_in_use;
#elif defined(__GLIBC__) && (__GLIBC__ > 2 || (__GLIBC__ == 2 && __GLIBC_MINOR__ >= 33))
    return mallinfo2().uordblks;
#elif defined(__GLIBC__)
    return static_cast<unsigned int>(mallinfo().uordblks);
#else
    return 0;
#endif
}

}
