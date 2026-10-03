#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>

namespace tarantino::crawler::benchmarking {

/// The memory of the benchmark process (SPEC §11), as Java's MemoryMXBean and ThreadMXBean give it.
class Heap {
public:
    Heap() = delete;

    /// Bytes in use by the allocator now, with no collection needed: mallinfo2().uordblks on Linux,
    /// malloc_zone_statistics's size_in_use on macOS and the busy blocks of the process heaps on Windows; 0 where the
    /// allocator tells nothing.
    static std::uint64_t inUse();

    /// Bytes requested from operator new so far by every thread, garbage included: counted by the operator new that the
    /// benchmark executables replace (AllocationCounter.cpp), 0 elsewhere.
    static std::uint64_t allocatedSoFar() {
        return allocated_.load(std::memory_order_relaxed);
    }

    static void countAllocation(std::size_t bytes) {
        allocated_.fetch_add(bytes, std::memory_order_relaxed);
    }

private:
    static std::atomic<std::uint64_t> allocated_;
};

}
