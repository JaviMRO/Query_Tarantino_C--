// The global operator new of the benchmark executables, replaced to count every byte requested (SPEC §11,
// memory_allocated), as Java's ThreadMXBean.getTotalThreadAllocatedBytes counts them. Linked into each benchmark
// executable as an object file, never into the services.

#include "crawler/tests/benchmarking/support/environment/Heap.hpp"

#include <cstdlib>
#include <new>

#ifdef _WIN32
#include <malloc.h>
#endif

namespace {

using tarantino::crawler::benchmarking::Heap;

void* allocate(std::size_t size) {
    Heap::countAllocation(size);
    return std::malloc(size == 0 ? 1 : size);
}

void* allocateAligned(std::size_t size, std::align_val_t alignment) {
    Heap::countAllocation(size);
    const auto bytes = static_cast<std::size_t>(alignment);
#ifdef _WIN32
    return _aligned_malloc(size == 0 ? 1 : size, bytes);
#else
    void* memory = nullptr;
    return posix_memalign(&memory, bytes < sizeof(void*) ? sizeof(void*) : bytes, size == 0 ? 1 : size) == 0 ? memory : nullptr;
#endif
}

void releaseAligned(void* memory) {
#ifdef _WIN32
    _aligned_free(memory);
#else
    std::free(memory);
#endif
}

}

void* operator new(std::size_t size) {
    if (void* memory = allocate(size)) {
        return memory;
    }
    throw std::bad_alloc();
}

void* operator new[](std::size_t size) {
    if (void* memory = allocate(size)) {
        return memory;
    }
    throw std::bad_alloc();
}

void* operator new(std::size_t size, const std::nothrow_t&) noexcept {
    return allocate(size);
}

void* operator new[](std::size_t size, const std::nothrow_t&) noexcept {
    return allocate(size);
}

void* operator new(std::size_t size, std::align_val_t alignment) {
    if (void* memory = allocateAligned(size, alignment)) {
        return memory;
    }
    throw std::bad_alloc();
}

void* operator new[](std::size_t size, std::align_val_t alignment) {
    if (void* memory = allocateAligned(size, alignment)) {
        return memory;
    }
    throw std::bad_alloc();
}

void* operator new(std::size_t size, std::align_val_t alignment, const std::nothrow_t&) noexcept {
    return allocateAligned(size, alignment);
}

void* operator new[](std::size_t size, std::align_val_t alignment, const std::nothrow_t&) noexcept {
    return allocateAligned(size, alignment);
}

void operator delete(void* memory) noexcept {
    std::free(memory);
}

void operator delete[](void* memory) noexcept {
    std::free(memory);
}

void operator delete(void* memory, std::size_t) noexcept {
    std::free(memory);
}

void operator delete[](void* memory, std::size_t) noexcept {
    std::free(memory);
}

void operator delete(void* memory, const std::nothrow_t&) noexcept {
    std::free(memory);
}

void operator delete[](void* memory, const std::nothrow_t&) noexcept {
    std::free(memory);
}

void operator delete(void* memory, std::align_val_t) noexcept {
    releaseAligned(memory);
}

void operator delete[](void* memory, std::align_val_t) noexcept {
    releaseAligned(memory);
}

void operator delete(void* memory, std::size_t, std::align_val_t) noexcept {
    releaseAligned(memory);
}

void operator delete[](void* memory, std::size_t, std::align_val_t) noexcept {
    releaseAligned(memory);
}

void operator delete(void* memory, std::align_val_t, const std::nothrow_t&) noexcept {
    releaseAligned(memory);
}

void operator delete[](void* memory, std::align_val_t, const std::nothrow_t&) noexcept {
    releaseAligned(memory);
}
