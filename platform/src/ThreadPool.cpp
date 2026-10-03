#include "tarantino/platform/ThreadPool.hpp"

#include <stdexcept>

namespace tarantino::platform {

ThreadPool::ThreadPool(int threads) {
    if (threads <= 0) {
        throw std::invalid_argument("A thread pool needs at least one thread");
    }
    threads_.reserve(static_cast<std::size_t>(threads));
    for (int thread = 0; thread < threads; ++thread) {
        threads_.emplace_back([this] { work(); });
    }
}

ThreadPool::~ThreadPool() {
    {
        const std::lock_guard lock(mutex_);
        stopping_ = true;
        tasks_.clear();
    }
    available_.notify_all();
    for (std::thread& thread : threads_) {
        thread.join();
    }
}

void ThreadPool::execute(std::function<void()> task) {
    {
        const std::lock_guard lock(mutex_);
        tasks_.push_back(std::move(task));
    }
    available_.notify_one();
}

void ThreadPool::work() {
    while (true) {
        std::function<void()> task;
        {
            std::unique_lock lock(mutex_);
            available_.wait(lock, [this] { return stopping_ || !tasks_.empty(); });
            if (stopping_) {
                return;
            }
            task = std::move(tasks_.front());
            tasks_.pop_front();
        }
        task();
    }
}

}
